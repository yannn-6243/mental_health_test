#include "crow_all.h"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>

sqlite3* db;

void initDatabase() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            name TEXT,
            score INTEGER NOT NULL,
            max_score INTEGER NOT NULL,
            category TEXT NOT NULL,
            note TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

std::string getCurrentTimestamp() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d/%m/%Y, %H:%M:%S");
    return oss.str();
}

int main() {
    if (sqlite3_open("mental_health.db", &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    initDatabase();
    
    crow::SimpleApp app;

    // CORS middleware
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization")
        .methods("GET"_method, "POST"_method, "DELETE"_method, "OPTIONS"_method)
        .origin("*");

    // Health check
    CROW_ROUTE(app, "/")
    ([]() {
        return crow::response(200, "Mental Health API is running!");
    });

    CROW_ROUTE(app, "/api/health")
    ([]() {
        crow::json::wvalue res;
        res["status"] = "ok";
        res["message"] = "Backend C++ is running";
        return crow::response(200, res);
    });

    // Submit test result
    CROW_ROUTE(app, "/api/submit").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string name = body.has("name") ? body["name"].s() : "";
        int score = body["score"].i();
        int maxScore = body["max_score"].i();
        std::string category = body["category"].s();
        std::string note = body.has("note") ? body["note"].s() : "";
        std::string timestamp = getCurrentTimestamp();

        const char* sql = "INSERT INTO history (timestamp, name, score, max_score, category, note) VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return crow::response(500, "Database error");
        }

        sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, score);
        sqlite3_bind_int(stmt, 4, maxScore);
        sqlite3_bind_text(stmt, 5, category.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, note.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return crow::response(500, "Failed to save data");
        }

        int lastId = sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);

        crow::json::wvalue res;
        res["success"] = true;
        res["id"] = lastId;
        res["timestamp"] = timestamp;
        return crow::response(201, res);
    });

    // Get history
    CROW_ROUTE(app, "/api/history")
    ([]() {
        const char* sql = "SELECT id, timestamp, name, score, max_score, category, note FROM history ORDER BY id DESC;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return crow::response(500, "Database error");
        }

        std::vector<crow::json::wvalue> records;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            crow::json::wvalue record;
            record["id"] = sqlite3_column_int(stmt, 0);
            record["timestamp"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            record["name"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            record["score"] = sqlite3_column_int(stmt, 3);
            record["max_score"] = sqlite3_column_int(stmt, 4);
            record["category"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            
            const char* noteText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            record["note"] = noteText ? noteText : "";
            
            records.push_back(std::move(record));
        }
        sqlite3_finalize(stmt);

        crow::json::wvalue res;
        res["data"] = std::move(records);
        return crow::response(200, res);
    });

    // Delete single record
    CROW_ROUTE(app, "/api/history/<int>").methods("DELETE"_method)
    ([](int id) {
        const char* sql = "DELETE FROM history WHERE id = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return crow::response(500, "Database error");
        }

        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        int changes = sqlite3_changes(db);
        sqlite3_finalize(stmt);

        if (changes == 0) {
            return crow::response(404, "Record not found");
        }

        crow::json::wvalue res;
        res["success"] = true;
        res["deleted_id"] = id;
        return crow::response(200, res);
    });

    // Delete all history
    CROW_ROUTE(app, "/api/history").methods("DELETE"_method)
    ([]() {
        const char* sql = "DELETE FROM history;";
        char* errMsg = nullptr;

        if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            return crow::response(500, error);
        }

        crow::json::wvalue res;
        res["success"] = true;
        res["message"] = "All history deleted";
        return crow::response(200, res);
    });

    // Get port from environment variable (Railway sets this)
    char* port_str = std::getenv("PORT");
    int port = port_str ? std::stoi(port_str) : 8080;

    std::cout << "Starting server on port " << port << std::endl;
    app.port(port).bindaddr("0.0.0.0").multithreaded().run();

    sqlite3_close(db);
    return 0;
}
