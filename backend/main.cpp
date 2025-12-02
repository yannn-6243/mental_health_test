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

crow::response addCorsHeaders(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
    return res;
}

int main() {
    if (sqlite3_open("mental_health.db", &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    initDatabase();
    
    crow::SimpleApp app;

    // Health check
    CROW_ROUTE(app, "/")
    ([]() {
        crow::response res("Mental Health API is running!");
        return addCorsHeaders(res);
    });

    CROW_ROUTE(app, "/api/health")
    ([]() {
        crow::json::wvalue data;
        data["status"] = "ok";
        data["message"] = "Backend C++ is running";
        crow::response res(data);
        return addCorsHeaders(res);
    });

    // Submit test result
    CROW_ROUTE(app, "/api/submit").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req) {
        // Handle CORS preflight
        if (req.method == "OPTIONS"_method) {
            crow::response res(204);
            return addCorsHeaders(res);
        }

        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400, "Invalid JSON");
            return addCorsHeaders(res);
        }

        std::string name = "";
        if (body.has("name")) {
            name = std::string(body["name"].s());
        }
        
        int score = body["score"].i();
        int maxScore = body["max_score"].i();
        std::string category = std::string(body["category"].s());
        
        std::string note = "";
        if (body.has("note")) {
            note = std::string(body["note"].s());
        }
        
        std::string timestamp = getCurrentTimestamp();

        const char* sql = "INSERT INTO history (timestamp, name, score, max_score, category, note) VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            crow::response res(500, "Database error");
            return addCorsHeaders(res);
        }

        sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, score);
        sqlite3_bind_int(stmt, 4, maxScore);
        sqlite3_bind_text(stmt, 5, category.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, note.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            crow::response res(500, "Failed to save data");
            return addCorsHeaders(res);
        }

        int lastId = sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);

        crow::json::wvalue result;
        result["success"] = true;
        result["id"] = lastId;
        result["timestamp"] = timestamp;
        
        crow::response res(201, result);
        return addCorsHeaders(res);
    });

    // Get and Delete history (combined to avoid duplicate route error)
    CROW_ROUTE(app, "/api/history").methods("GET"_method, "DELETE"_method, "OPTIONS"_method)
    ([](const crow::request& req) {
        // Handle CORS preflight
        if (req.method == "OPTIONS"_method) {
            crow::response res(204);
            return addCorsHeaders(res);
        }

        // GET - fetch history
        if (req.method == "GET"_method) {
            const char* sql = "SELECT id, timestamp, name, score, max_score, category, note FROM history ORDER BY id DESC;";
            sqlite3_stmt* stmt;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
                crow::response res(500, "Database error");
                return addCorsHeaders(res);
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

            crow::json::wvalue result;
            result["data"] = std::move(records);
            
            crow::response res(200, result);
            return addCorsHeaders(res);
        }

        // DELETE - clear all history
        if (req.method == "DELETE"_method) {
            const char* sql = "DELETE FROM history;";
            char* errMsg = nullptr;

            if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
                std::string error = errMsg;
                sqlite3_free(errMsg);
                crow::response res(500, error);
                return addCorsHeaders(res);
            }

            crow::json::wvalue result;
            result["success"] = true;
            result["message"] = "All history deleted";
            
            crow::response res(200, result);
            return addCorsHeaders(res);
        }

        crow::response res(405, "Method not allowed");
        return addCorsHeaders(res);
    });

    // Get port from environment variable (Railway sets this)
    char* port_str = std::getenv("PORT");
    int port = port_str ? std::stoi(port_str) : 8080;

    std::cout << "Starting server on port " << port << std::endl;
    app.port(port).bindaddr("0.0.0.0").multithreaded().run();

    sqlite3_close(db);
    return 0;
}
