// ========================================
// API Configuration
// ========================================
const API_BASE_URL = 'https://mentalhealthtest-production.up.railway.app';

// Backend is deployed on Railway
const USE_BACKEND = true;

// ========================================
// WASM init
// ========================================
let classifyWasm = null;
Module().then(instance => {
  classifyWasm = instance.cwrap('classify', 'string', ['number', 'number']);
});

// ========================================
// Sections
// ========================================
const sections = {
  home: document.getElementById('homeSection'),
  test: document.getElementById('testSection'),
  result: document.getElementById('resultSection'),
  history: document.getElementById('historySection'),
  about: document.getElementById('aboutSection'),
};

// Nav indicator
const navButtons = document.querySelectorAll('#mainNav .nav-link');
const navIndicator = document.getElementById('navIndicator');
const mainNav = document.getElementById('mainNav');

function moveIndicatorTo(button){
  if(!button || !navIndicator || !mainNav) return;
  const navRect = mainNav.getBoundingClientRect();
  const btnRect = button.getBoundingClientRect();
  const left = btnRect.left - navRect.left;
  const width = btnRect.width;
  navIndicator.style.opacity = 1;
  navIndicator.style.width = width + 'px';
  navIndicator.style.transform = `translateX(${left}px)`;
}

function setActiveNav(section){
  let activeBtn = null;
  navButtons.forEach(btn=>{
    const isActive = btn.getAttribute('data-nav') === section;
    btn.classList.toggle('nav-link-active', isActive);
    if(isActive) activeBtn = btn;
  });
  if(activeBtn) moveIndicatorTo(activeBtn);
}

function show(section){
  Object.entries(sections).forEach(([key, el])=>{
    const isActive = key === section;
    if(isActive){
      el.classList.remove('hidden');
      requestAnimationFrame(()=> el.classList.add('active'));
    }else{
      el.classList.remove('active');
      el.classList.add('hidden');
    }
  });

  setActiveNav(section);
  if(section==='history') renderHistory();
  if(section==='test') startTest();
}

// bind nav clicks
document.querySelectorAll('[data-nav]').forEach(btn=>{
  btn.addEventListener('click', ()=> show(btn.getAttribute('data-nav')));
});
window.addEventListener('resize', ()=>{
  const current = document.querySelector('#mainNav .nav-link.nav-link-active');
  if(current) moveIndicatorTo(current);
});

// ========================================
// Questions (20 items)
// ========================================
const questions = [
  { text: "Saya merasa cemas tanpa alasan yang jelas.", reverse: false },
  { text: "Saya kesulitan tidur atau sering terbangun di malam hari.", reverse: false },
  { text: "Saya merasa bersemangat menjalani hari.", reverse: true },
  { text: "Saya kehilangan minat pada hal-hal yang biasanya saya sukai.", reverse: false },
  { text: "Saya merasa tenang dan rileks.", reverse: true },
  { text: "Saya merasa tidak berharga atau gagal.", reverse: false },
  { text: "Saya mudah tersinggung atau marah.", reverse: false },
  { text: "Saya merasa puas dengan diri saya.", reverse: true },
  { text: "Saya merasa sedih atau hampa hampir setiap hari.", reverse: false },
  { text: "Saya merasa percaya diri dan mampu menghadapi tantangan.", reverse: true },
  { text: "Saya merasa lelah meskipun tidak banyak aktivitas.", reverse: false },
  { text: "Saya merasa terisolasi atau kesepian.", reverse: false },
  { text: "Saya merasa optimis tentang masa depan.", reverse: true },
  { text: "Saya merasa sulit berkonsentrasi atau fokus.", reverse: false },
  { text: "Saya merasa nyaman berada di sekitar orang lain.", reverse: true },
  { text: "Saya merasa bersalah atau menyesal berlebihan.", reverse: false },
  { text: "Saya merasa mampu mengelola stres dengan baik.", reverse: true },
  { text: "Saya merasa tidak punya harapan atau tujuan hidup.", reverse: false },
  { text: "Saya merasa bangga atas pencapaian saya.", reverse: true },
  { text: "Saya merasa ingin menghindari interaksi sosial.", reverse: false }
];

let step = 0;
let answers = Array(questions.length).fill(null);

// UI refs
const stepText = document.getElementById('stepText');
const questionText = document.getElementById('questionText');
const progressEl = document.getElementById('progress');
const nameInput = document.getElementById('nameInput');
const noteInput = document.getElementById('noteInput');

function startTest(){
  step = 0;
  answers = Array(questions.length).fill(null);
  nameInput.value = '';
  noteInput.value = '';
  document.getElementById('resMax').textContent = questions.length * 3;
  renderStep();
}

function renderStep(){
  stepText.textContent = (step + 1);
  questionText.textContent = `(${step+1}). ${questions[step].text}`;
  progressEl.style.width = (step / (questions.length - 1)) * 100 + '%';
  document.querySelectorAll('input[name="scale"]').forEach(r=>{
    r.checked = (answers[step] !== null && +r.value === answers[step]);
  });
}

function getSelected(){
  const r = document.querySelector('input[name="scale"]:checked');
  return r ? +r.value : null;
}

document.getElementById('prevBtn').addEventListener('click', ()=>{
  if(step > 0){ step--; renderStep(); }
});

document.getElementById('nextBtn').addEventListener('click', ()=>{
  const sel = getSelected();
  if(sel === null){ alert('Pilih nilai 0–3 dulu ya.'); return; }
  answers[step] = sel;
  if(step < questions.length - 1){ step++; renderStep(); }
  else finishTest();
});

// ========================================
// API Functions
// ========================================
async function submitToBackend(data) {
  try {
    const response = await fetch(`${API_BASE_URL}/api/submit`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(data)
    });
    
    if (!response.ok) {
      throw new Error('Failed to submit');
    }
    
    return await response.json();
  } catch (error) {
    console.error('Backend error:', error);
    return null;
  }
}

async function getHistoryFromBackend() {
  try {
    const response = await fetch(`${API_BASE_URL}/api/history`);
    
    if (!response.ok) {
      throw new Error('Failed to fetch history');
    }
    
    const result = await response.json();
    return result.data || [];
  } catch (error) {
    console.error('Backend error:', error);
    return null;
  }
}

async function deleteAllFromBackend() {
  try {
    const response = await fetch(`${API_BASE_URL}/api/history`, {
      method: 'DELETE'
    });
    
    if (!response.ok) {
      throw new Error('Failed to delete');
    }
    
    return await response.json();
  } catch (error) {
    console.error('Backend error:', error);
    return null;
  }
}

// ========================================
// Local Storage (Fallback)
// ========================================
const STORAGE_KEY = 'mentalHealthHistorySDG3';

function getHistoryLocal(){
  const json = localStorage.getItem(STORAGE_KEY);
  return json ? JSON.parse(json) : [];
}

function saveHistoryLocal(item){
  const hist = getHistoryLocal();
  hist.unshift(item);
  localStorage.setItem(STORAGE_KEY, JSON.stringify(hist));
}

function clearHistoryLocal(){
  localStorage.removeItem(STORAGE_KEY);
}

// ========================================
// Finish Test
// ========================================
async function finishTest(){
  if(answers.some(a => a === null)){
    alert('Jawab semua pertanyaan dulu ya.');
    return;
  }
  
  const total = answers.reduce((sum, val, i) => {
    const rev = questions[i].reverse;
    return sum + (rev ? 3 - val : val);
  }, 0);
  const maxScore = questions.length * 3;

  // Classify using WASM or fallback
  let cat = 'Error';
  let advice = 'Skor tidak valid.';
  let color = '#888888';

  if(typeof classifyWasm === 'function'){
    cat = classifyWasm(total, maxScore);
  }else{
    if(total < 0 || total > maxScore){
      cat = 'Error'; advice = 'Skor tidak valid.'; color = '#888888';
    } else if (total <= Math.floor(maxScore * 0.33)) {
      cat = 'Baik'; advice = 'Pertahankan pola hidup sehat, teruskan refleksi diri.'; color = '#16a34a';
    } else if (total <= Math.floor(maxScore * 0.66)) {
      cat = 'Perlu Perhatian Ringan'; advice = 'Coba relaksasi, atur jadwal, dan cukup tidur.'; color = '#f59e0b';
    } else {
      cat = 'Disarankan Konsultasi'; advice = 'Pertimbangkan segera berkonsultasi dengan profesional.'; color = '#ef4444';
    }
  }

  const map = {
    'Baik': { advice: 'Pertahankan pola hidup sehat, teruskan refleksi diri.', color: '#16a34a' },
    'Perlu Perhatian Ringan': { advice: 'Coba relaksasi, atur jadwal, dan cukup tidur.', color: '#f59e0b' },
    'Disarankan Konsultasi': { advice: 'Pertimbangkan segera berkonsultasi dengan profesional.', color: '#ef4444' },
    'Error': { advice: 'Skor tidak valid.', color: '#888888' }
  };
  const picked = map[cat] || { advice, color };

  // Show result
  document.getElementById('resName').textContent = nameInput.value.trim() || '-';
  document.getElementById('resScore').textContent = total;
  const catEl = document.getElementById('resCategory');
  catEl.textContent = cat;
  catEl.style.color = picked.color;
  document.getElementById('resAdvice').textContent = picked.advice;

  // Save to backend or localStorage
  const historyItem = {
    name: nameInput.value.trim() || '-',
    score: total,
    max_score: maxScore,
    category: cat,
    note: noteInput.value.trim() || ''
  };

  if (USE_BACKEND) {
    const result = await submitToBackend(historyItem);
    if (!result) {
      // Fallback to localStorage if backend fails
      saveHistoryLocal({
        timestamp: new Date().toLocaleString(),
        name: historyItem.name,
        total: historyItem.score,
        category: historyItem.category,
        note: historyItem.note
      });
      console.log('Saved to localStorage (backend unavailable)');
    }
  } else {
    saveHistoryLocal({
      timestamp: new Date().toLocaleString(),
      name: historyItem.name,
      total: historyItem.score,
      category: historyItem.category,
      note: historyItem.note
    });
  }

  show('result');
}

document.getElementById('restartBtn')?.addEventListener('click', ()=> show('test'));

// ========================================
// History Rendering
// ========================================
async function renderHistory(){
  const body = document.getElementById('historyBody');
  body.innerHTML = '<tr><td class="py-3 px-2" colspan="5">Memuat data...</td></tr>';

  let rows = [];
  
  if (USE_BACKEND) {
    const backendData = await getHistoryFromBackend();
    if (backendData !== null) {
      // Transform backend data format
      rows = backendData.map(r => ({
        id: r.id,
        timestamp: r.timestamp,
        name: r.name,
        total: r.score,
        category: r.category,
        note: r.note || ''
      }));
    } else {
      // Fallback to localStorage
      rows = getHistoryLocal();
    }
  } else {
    rows = getHistoryLocal();
  }

  body.innerHTML = '';

  if(!rows.length){
    body.innerHTML = '<tr><td class="py-3 px-2 text-emerald-700 dark:text-emerald-300" colspan="5">Belum ada data. Lakukan tes terlebih dahulu.</td></tr>';
    renderStats(rows);
    drawTrend(rows);
    return;
  }

  rows.forEach(r=>{
    const tr = document.createElement('tr');
    tr.className = "border-t border-emerald-100 dark:border-emerald-700";
    tr.innerHTML = `
      <td class='py-1.5 px-2 whitespace-nowrap'>${r.timestamp}</td>
      <td class='py-1.5 px-2'>${r.name}</td>
      <td class='py-1.5 px-2 font-semibold text-emerald-700 dark:text-emerald-400'>${r.total}</td>
      <td class='py-1.5 px-2'>${r.category}</td>
      <td class='py-1.5 px-2'>${r.note}</td>
    `;
    body.appendChild(tr);
  });

  renderStats(rows);
  drawTrend(rows);
}

function renderStats(rows){
  const ul = document.getElementById('statsList');
  ul.innerHTML = '';
  if(!rows.length){
    ul.innerHTML = '<li>Belum ada data. Lakukan tes minimal sekali.</li>';
    return;
  }
  const scores = rows.map(r=> r.total);
  const n = scores.length;
  const sum = scores.reduce((a,b)=>a+b,0);
  const mn = Math.min(...scores);
  const mx = Math.max(...scores);
  const avg = (sum/n).toFixed(2);
  ul.innerHTML = `
    <li>Jumlah entri: <b>${n}</b></li>
    <li>Skor minimum: <b>${mn}</b></li>
    <li>Skor maksimum: <b>${mx}</b></li>
    <li>Rata-rata: <b>${avg}</b></li>
  `;
}

function drawTrend(rows){
  const canvas = document.getElementById('trendCanvas');
  const ctx = canvas.getContext('2d');
  ctx.clearRect(0,0,canvas.width,canvas.height);

  if(!rows.length){
    ctx.fillStyle='#4b5563';
    ctx.font='12px system-ui';
    ctx.fillText('Belum ada data untuk digambarkan.', 10, 20);
    return;
  }

  const rowsToShow = rows.slice(0,10).reverse();
  const w = canvas.width, h = canvas.height;
  const pad = 30, barGap = 8;
  const n = rowsToShow.length;
  const barW = (w - pad*2 - (n-1)*barGap)/n;

  ctx.strokeStyle = '#d1fae5';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(pad, pad);
  ctx.lineTo(pad, h-pad);
  ctx.lineTo(w-pad, h-pad);
  ctx.stroke();

  ctx.fillStyle = '#6b7280';
  ctx.font = '11px system-ui';
  ctx.fillText(String(questions.length*3), 6, pad+4);
  ctx.fillText('0', 12, h-pad+11);

  rowsToShow.forEach((r,i)=>{
    const x = pad + i*(barW+barGap);
    const barH = (r.total/(questions.length*3))*(h - pad*2);
    const color = r.total <= Math.floor(questions.length*3*0.33) ? '#22c55e'
                 : r.total <= Math.floor(questions.length*3*0.66) ? '#fbbf24'
                 : '#f97373';
    ctx.fillStyle = color;
    if(typeof ctx.roundRect === 'function'){
      ctx.beginPath();
      ctx.roundRect(x, h-pad-barH, barW, barH, 4);
      ctx.fill();
    }else{
      ctx.fillRect(x, h-pad-barH, barW, barH);
    }
  });
}

// ========================================
// Export & Clear
// ========================================
document.getElementById('exportCsv').addEventListener('click', async ()=>{
  let rows = [];
  
  if (USE_BACKEND) {
    const backendData = await getHistoryFromBackend();
    if (backendData !== null) {
      rows = backendData.map(r => ({
        timestamp: r.timestamp,
        name: r.name,
        total: r.score,
        category: r.category,
        note: r.note || ''
      }));
    } else {
      rows = getHistoryLocal();
    }
  } else {
    rows = getHistoryLocal();
  }

  let csv = 'Tanggal,Nama,Skor,Kategori,Catatan\n';
  rows.forEach(r=>{
    csv += `"${r.timestamp}","${r.name}",${r.total},"${r.category}","${r.note}"\n`;
  });
  const blob = new Blob([csv], { type: 'text/csv' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'riwayat_kesehatan_mental.csv';
  a.click();
  URL.revokeObjectURL(url);
});

document.getElementById('clearAll').addEventListener('click', async ()=>{
  if(!confirm('Anda yakin ingin MENGHAPUS SEMUA riwayat? Tindakan ini tidak dapat dibatalkan.')) return;
  
  if (USE_BACKEND) {
    const result = await deleteAllFromBackend();
    if (!result) {
      // Also clear localStorage as fallback
      clearHistoryLocal();
    }
  } else {
    clearHistoryLocal();
  }
  
  renderHistory();
});

// ========================================
// Initialize
// ========================================
show('home');

// Check backend connection on load
if (USE_BACKEND) {
  fetch(`${API_BASE_URL}/api/health`)
    .then(res => res.json())
    .then(data => console.log('Backend connected:', data))
    .catch(err => console.warn('Backend not available, will use localStorage fallback'));
}
