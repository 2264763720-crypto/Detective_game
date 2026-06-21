// ============================================================
//  雨夜密室杀人案 - 游戏引擎
//  侦探推理游戏 · 浏览器版
// ============================================================

// ---------- 游戏状态 ----------
const state = {
    currentSceneId: 1,
    hp: 100,
    clue: 0,
    items: [],
    suspectTrust: [50, 50, 50, 50, 50],
    visitedScenes: [],
    accusedSuspects: [],
    startTime: Date.now(),
    accusedCount: 0
};

// ---------- 实时计时器 ----------
let timerInterval = null;

function startTimer() {
    if (timerInterval) clearInterval(timerInterval);
    timerInterval = setInterval(updateTimer, 1000);
}

function stopTimer() {
    if (timerInterval) { clearInterval(timerInterval); timerInterval = null; }
}

function updateTimer() {
    const elapsed = Math.floor((Date.now() - state.startTime) / 1000);
    const m = Math.floor(elapsed / 60);
    const s = elapsed % 60;
    document.getElementById('time-text').textContent =
        `${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`;
}

// ---------- 工具函数 ----------
function findScene(id) {
    return STORY_DATA.scenes.find(s => s.id === id);
}

function hasItem(name) {
    if (!name) return true;
    return state.items.some(item => item.name === name);
}

function addItem(name) {
    if (!name) return;
    if (hasItem(name)) return;
    state.items.push({ name, description: '' });
}

function isSceneVisited(id) {
    return state.visitedScenes.includes(id);
}

function recordVisit(id) {
    if (!isSceneVisited(id)) state.visitedScenes.push(id);
}

// ---------- 选项过滤 ----------
function isChoiceAvailable(choice) {
    if (choice.item_required && !hasItem(choice.item_required)) return false;
    if (choice.is_hidden && choice.hidden_min_clue > state.clue) return false;
    if (choice.required_visited) {
        for (const vid of choice.required_visited) {
            if (vid > 0 && !isSceneVisited(vid)) return false;
        }
    }
    return true;
}

function getAvailableChoices(scene) {
    const available = [];
    for (let i = 0; i < scene.choices.length; i++) {
        if (isChoiceAvailable(scene.choices[i])) available.push(i);
    }
    return available;
}

// ---------- 随机事件 ----------
function handleRandomEvent(scene) {
    if (!scene.is_random) return;
    const roll = Math.random() * 100;

    if (roll < 5 && state.clue < 20) {
        state.hp = 0;
        showToast('☠️ 暗杀！黑影冲出，匕首刺入胸口...生命值归零！', 'danger');
    } else if (roll < 30) {
        state.clue = Math.min(100, state.clue + 5);
        showToast('🔍 意外发现被忽略的细节！线索 +5', 'info');
    } else if (roll < 55) {
        state.hp = Math.max(0, state.hp - 10);
        showToast('⚡ 触发了警报装置，受到电击！HP -10', 'warning');
    } else if (roll < 70) {
        const randomItems = ['破碎的玻璃片', '旧报纸碎片', '可疑的收据'];
        const item = randomItems[Math.floor(Math.random() * 3)];
        addItem(item);
        showToast(`📦 在角落发现了东西！获得: ${item}`, 'info');
    } else if (roll < 80) {
        state.hp = Math.max(0, state.hp - 3);
        showToast('🤕 调查中不慎摔倒。HP -3', 'warning');
    }
}

// ---------- Toast 提示 ----------
let toastTimeout = null;
function showToast(msg, type) {
    const toast = document.getElementById('toast');
    if (toastTimeout) clearTimeout(toastTimeout);
    toast.textContent = msg;
    toast.className = `toast ${type} show`;
    toastTimeout = setTimeout(() => { toast.className = 'toast'; }, 2800);
}

// ---------- 场景内容渲染（不含选项） ----------
function renderSceneContent(scene) {
    const locEl = document.getElementById('scene-location');
    const atmoEl = document.getElementById('scene-atmosphere');
    const descEl = document.getElementById('scene-description');

    locEl.textContent = scene.location;
    const atm = scene.atmosphere;
    atmoEl.textContent = atm;
    atmoEl.className = 'scene-atmo';
    if (atm === '紧张') atmoEl.classList.add('tense');
    else if (atm === '危险') atmoEl.classList.add('danger');
    else if (atm === '悬疑') atmoEl.classList.add('suspense');
    else if (atm === '发现') atmoEl.classList.add('discover');
    else atmoEl.classList.add('investigate');

    descEl.className = 'scene-text';
    if (atm === '紧张' || atm === '危险') descEl.classList.add('danger');
    else if (atm === '悬疑') descEl.classList.add('suspense');
    else if (atm === '发现') descEl.classList.add('discover');
    else if (atm === '调查') descEl.classList.add('tense');
    descEl.textContent = scene.description;
}

// ---------- 场景渲染 ----------
function renderScene() {
    const scene = findScene(state.currentSceneId);
    if (!scene) {
        document.getElementById('scene-description').textContent = '错误：场景未找到';
        return;
    }

    recordVisit(state.currentSceneId);
    handleRandomEvent(scene);

    if (state.hp <= 0) {
        showEnding('escape');
        return;
    }

    // 结局场景处理
    if (scene.is_ending) {
        if (state.currentSceneId === 30) {
            renderSceneContent(scene);
            updateStatusPanel();
            setTimeout(() => showAccusationModal(), 500);
            return;
        }
        if (state.currentSceneId === 100) {
            processEnding();
            return;
        }
    }

    // 渲染场景内容
    renderSceneContent(scene);

    // --- 渲染选项 ---
    const choicesContainer = document.getElementById('choices-container');
    choicesContainer.innerHTML = '';
    const available = getAvailableChoices(scene);

    if (available.length === 0) {
        choicesContainer.innerHTML =
            '<div style="color:#f87171;text-align:center;padding:20px">⚠ 当前没有可用的选项。游戏无法继续。</div>';
        updateStatusPanel();
        return;
    }

    for (const idx of available) {
        const ch = scene.choices[idx];
        const btn = document.createElement('button');
        btn.className = 'choice-btn';
        btn.innerHTML = `
            <span class="choice-num">${idx + 1}</span>
            <div class="choice-info">
                <div class="choice-label">${ch.text}</div>
                <div class="choice-meta">
                    ${ch.hp_change !== 0 ? `<span class="meta-tag ${ch.hp_change > 0 ? 'meta-hp-gain' : 'meta-hp-loss'}">❤ ${ch.hp_change > 0 ? '+' : ''}${ch.hp_change}</span>` : ''}
                    ${ch.clue_change !== 0 ? `<span class="meta-tag meta-clue">🔍 ${ch.clue_change > 0 ? '+' : ''}${ch.clue_change}</span>` : ''}
                    ${ch.item_gain ? `<span class="meta-tag meta-item">📦 ${ch.item_gain}</span>` : ''}
                </div>
            </div>`;
        btn.onclick = () => processChoice(idx);
        choicesContainer.appendChild(btn);
    }

    // 不可用选项
    for (let i = 0; i < scene.choices.length; i++) {
        if (available.includes(i)) continue;
        const ch = scene.choices[i];
        const reasons = [];
        if (ch.item_required && !hasItem(ch.item_required)) reasons.push(`需要道具: ${ch.item_required}`);
        if (ch.is_hidden && ch.hidden_min_clue > state.clue) reasons.push(`需要线索 ≥${ch.hidden_min_clue}`);
        if (ch.required_visited) {
            for (const vid of ch.required_visited) {
                if (vid > 0 && !isSceneVisited(vid)) {
                    const rs = findScene(vid);
                    reasons.push(`需先调查: ${rs ? rs.location : '关键场景'}`);
                }
            }
        }
        const div = document.createElement('div');
        div.className = 'choice-btn disabled';
        div.innerHTML = `
            <span class="choice-num">✕</span>
            <div class="choice-info">
                <div class="choice-label">${ch.text}</div>
                <div class="choice-meta"><span class="meta-tag meta-req">${reasons.join(' · ')}</span></div>
            </div>`;
        choicesContainer.appendChild(div);
    }

    updateStatusPanel();
}

// ---------- 处理选择 ----------
function processChoice(idx) {
    const scene = findScene(state.currentSceneId);
    if (!scene) return;
    const choice = scene.choices[idx];
    if (!isChoiceAvailable(choice)) return;

    // 应用效果
    state.hp = Math.min(100, Math.max(0, state.hp + choice.hp_change));
    state.clue = Math.min(100, Math.max(0, state.clue + choice.clue_change));

    if (choice.item_gain) {
        addItem(choice.item_gain);
        showToast(`✓ 获得新道具: ${choice.item_gain}`, 'success');
    }

    if (state.hp <= 0) {
        updateStatusPanel();
        showEnding('escape');
        return;
    }

    state.currentSceneId = choice.target_scene;
    document.getElementById('scene-card').scrollIntoView({ behavior: 'smooth' });
    renderScene();
}

// ---------- 状态面板 ----------
function updateStatusPanel() {
    // HP
    document.getElementById('hp-text').textContent = `${state.hp}/100`;
    const hpBar = document.getElementById('hp-bar');
    const hpGlow = document.getElementById('hp-glow');
    hpBar.style.width = `${state.hp}%`;
    hpBar.className = 'hp-bar-inner';
    hpGlow.style.width = `${state.hp}%`;
    if (state.hp <= 30) { hpBar.classList.add('danger'); }
    else if (state.hp <= 60) { hpBar.classList.add('warning'); }

    // 线索
    document.getElementById('clue-text').textContent = state.clue;
    document.getElementById('clue-fill').style.width = `${state.clue}%`;

    // 道具
    const itemsContainer = document.getElementById('items-container');
    if (state.items.length === 0) {
        itemsContainer.innerHTML = '<span class="no-items">— 暂无道具 —</span>';
    } else {
        itemsContainer.innerHTML = state.items.map(i => `<span class="item-chip">${i.name}</span>`).join('');
    }

    // 信任度
    const trustContainer = document.getElementById('trust-container');
    trustContainer.innerHTML = '';
    for (let i = 0; i < STORY_DATA.suspects.length; i++) {
        const t = state.suspectTrust[i];
        let cls = 'trust-mid';
        let barColor = '#fbbf24';
        if (t >= 60) { cls = 'trust-high'; barColor = '#4ade80'; }
        else if (t < 30) { cls = 'trust-low'; barColor = '#f87171'; }
        trustContainer.innerHTML += `
            <div class="trust-row">
                <span class="trust-name">${STORY_DATA.suspects[i].name}</span>
                <div class="trust-bar-wrap"><div class="trust-bar-fill" style="width:${t}%;background:${barColor}"></div></div>
                <span class="trust-pct ${cls}">${t}%</span>
            </div>`;
    }
}

// ---------- 嫌疑人档案弹窗 ----------
function showSuspects() {
    const modal = document.getElementById('suspect-modal');
    const list = document.getElementById('suspect-list');
    list.innerHTML = '';
    for (let i = 0; i < STORY_DATA.suspects.length; i++) {
        const s = STORY_DATA.suspects[i];
        list.innerHTML += `
            <div class="suspect-card">
                <h3>[${i + 1}] ${s.name}</h3>
                <div class="s-identity">${s.identity}</div>
                <div class="s-detail"><span>动机：</span>${s.motive}</div>
                <div class="s-detail"><span>不在场证明：</span>${s.alibi}</div>
                ${state.clue >= 40
                    ? `<div class="s-secret">🔓 ${s.secret}</div>`
                    : `<div class="s-secret-locked">🔒 隐藏秘密（线索值 ≥40 解锁）</div>`}
            </div>`;
    }
    modal.classList.add('active');
}

function closeSuspectModal() {
    document.getElementById('suspect-modal').classList.remove('active');
}

// ---------- 指认环节 ----------
let selectedAccusations = [];

function showAccusationModal() {
    const modal = document.getElementById('accuse-modal');
    const list = document.getElementById('accuse-list');
    const hint = document.getElementById('accuse-hint');
    const intro = document.getElementById('accuse-intro');

    selectedAccusations = [];
    recordVisit(30);

    intro.innerHTML = `
        <p style="margin-bottom:8px"><strong>经过漫长的调查，你已收集了大量线索。</strong></p>
        <p style="margin-bottom:8px">现在，是时候做出最终判断了。</p>
        <p style="color:#9498a8;font-size:0.82rem">提示：案件可能涉及多人，你可以选择 1 个或多个嫌疑人。</p>`;

    list.innerHTML = '';
    for (let i = 0; i < STORY_DATA.suspects.length; i++) {
        const s = STORY_DATA.suspects[i];
        const div = document.createElement('div');
        div.className = 'accuse-item';
        div.innerHTML = `
            <div class="accuse-check"></div>
            <div>
                <div class="accuse-name">${s.name}</div>
                <div class="accuse-motive">${s.identity} — ${s.motive}</div>
            </div>`;
        div.onclick = () => {
            const idx = selectedAccusations.indexOf(i);
            if (idx >= 0) {
                selectedAccusations.splice(idx, 1);
                div.classList.remove('selected');
                div.querySelector('.accuse-check').textContent = '';
            } else {
                selectedAccusations.push(i);
                div.classList.add('selected');
                div.querySelector('.accuse-check').textContent = '✓';
            }
        };
        list.appendChild(div);
    }

    if (state.clue >= 50) {
        hint.style.display = 'block';
        hint.innerHTML = `
            <strong>💡 推理提示（线索值 ≥50 解锁）</strong><br>
            · 毒药瓶上有谁的指纹？<br>
            · 神秘访客的真实目的是什么？<br>
            · 谁最有条件在酒中下毒？<br>
            · 密室的门锁是如何反锁的？`;
    } else {
        hint.style.display = 'none';
    }

    modal.classList.add('active');
}

function submitAccusation() {
    if (selectedAccusations.length === 0) {
        showToast('请至少选择一名嫌疑人', 'warning');
        return;
    }
    state.accusedSuspects = [...selectedAccusations];
    state.accusedCount = selectedAccusations.length;
    state.currentSceneId = 100;
    document.getElementById('accuse-modal').classList.remove('active');
    processEnding();
}

function skipAccusation() {
    state.accusedSuspects = [];
    state.accusedCount = 0;
    state.currentSceneId = 100;
    document.getElementById('accuse-modal').classList.remove('active');
    processEnding();
}

// ---------- 结局判定 ----------
function processEnding() {
    stopTimer();
    if (state.hp <= 0) { showEnding('escape'); return; }

    const candidates = [];
    for (const ending of STORY_DATA.endings) {
        if (state.clue < ending.min_clue) continue;
        if (ending.required_item && !hasItem(ending.required_item)) continue;
        if (ending.required_hp > 0 && state.hp < ending.required_hp) continue;
        if (ending.suspect_flags && ending.suspect_flags.length > 0) {
            if (state.accusedCount === 0) continue;
            let allFound = true;
            for (const flag of ending.suspect_flags) {
                if (!state.accusedSuspects.includes(flag)) { allFound = false; break; }
            }
            if (!allFound) continue;
        }
        candidates.push(ending);
    }

    const priority = { perfect: 0, twist: 1, partial: 2, wrong_accuse: 3, escape: 4 };
    candidates.sort((a, b) => priority[a.type] - priority[b.type]);
    showEnding(candidates[0] ? candidates[0].type : 'escape');
}

// ---------- 结局画面 ----------
function showEnding(type) {
    stopTimer();
    const ending = STORY_DATA.endings.find(e => e.type === type);
    if (!ending) return;

    const modal = document.getElementById('ending-modal');
    const typeEl = document.getElementById('ending-type');
    const descEl = document.getElementById('ending-description');
    const summaryEl = document.getElementById('ending-summary');

    const typeNames = {
        perfect: '★ 完美破案 ★',
        partial: '▲ 部分成功 ▲',
        wrong_accuse: '✘ 错判无辜 ✘',
        escape: '⚠ 真凶逃脱 ⚠',
        twist: '◆ 意外反转 ◆'
    };

    typeEl.textContent = typeNames[type] || '结局';
    typeEl.className = `ending-badge ${type}`;
    descEl.textContent = ending.description;

    const elapsed = Math.floor((Date.now() - state.startTime) / 1000);
    const m = Math.floor(elapsed / 60);
    const s = elapsed % 60;
    const timeStr = `${String(m).padStart(2, '0')}分${String(s).padStart(2, '0')}秒`;

    summaryEl.innerHTML = `
        <strong>── 调查总结 ──</strong><br>
        最终生命值：${state.hp}/100<br>
        收集线索：${state.clue} 点<br>
        持有道具：${state.items.length > 0 ? state.items.map(i => i.name).join('、') : '无'}<br>
        访问场景：${state.visitedScenes.length} 个<br>
        调查用时：${timeStr}<br>
        ${state.accusedCount > 0 ? '最终指认：' + state.accusedSuspects.map(i => STORY_DATA.suspects[i].name).join('、') : '未进行指认'}
    `;

    modal.classList.add('active');
}

// ---------- 重新开始 ----------
function restartGame() {
    stopTimer();
    state.currentSceneId = 1;
    state.hp = 100;
    state.clue = 0;
    state.items = [];
    state.suspectTrust = [50, 50, 50, 50, 50];
    state.visitedScenes = [];
    state.accusedSuspects = [];
    state.accusedCount = 0;
    state.startTime = Date.now();
    selectedAccusations = [];

    document.getElementById('ending-modal').classList.remove('active');
    document.getElementById('accuse-modal').classList.remove('active');
    document.getElementById('suspect-modal').classList.remove('active');

    startTimer();
    renderScene();
    document.getElementById('scene-card').scrollIntoView({ behavior: 'smooth' });
}

// ---------- 全局事件 ----------
// 点击遮罩关闭弹窗
document.addEventListener('click', e => {
    if (e.target.classList.contains('modal-overlay')) {
        e.target.classList.remove('active');
    }
});

// 键盘快捷键
document.addEventListener('keydown', e => {
    if (document.querySelector('.modal-overlay.active')) return;
    const scene = findScene(state.currentSceneId);
    if (!scene) return;
    const available = getAvailableChoices(scene);
    const key = parseInt(e.key);
    if (key >= 1 && key <= available.length) {
        processChoice(available[key - 1]);
    } else if (e.key === 's' || e.key === 'S') {
        showSuspects();
    }
});

// ---------- 启动游戏 ----------
startTimer();
renderScene();
