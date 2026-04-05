/* Downpour – torrent management JavaScript */
'use strict';

// ── State ────────────────────────────────────────────────────────────────────
const torrents = {};   // hash → row element
let pollTimer   = null;
let removeHash  = null;
let removeName_ = null;

// ── Utility ──────────────────────────────────────────────────────────────────
function fmtBytes(n) {
    if (n == null || n < 0) return '—';
    if (n < 1024)            return n + ' B';
    if (n < 1024 * 1024)     return (n / 1024).toFixed(1) + ' KB';
    if (n < 1024 ** 3)       return (n / 1024 / 1024).toFixed(2) + ' MB';
    return (n / 1024 / 1024 / 1024).toFixed(2) + ' GB';
}

function fmtSpeed(bps) {
    if (!bps) return '—';
    return fmtBytes(bps) + '/s';
}

function setStatus(msg, isError) {
    const el = document.getElementById('statusMsg');
    el.textContent = msg;
    el.style.color = isError ? '#e74c3c' : '#aaa';
}

// ── API helpers ───────────────────────────────────────────────────────────────
async function apiPost(url, params) {
    const body = new URLSearchParams(params);
    const res  = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body.toString()
    });
    return res.json();
}

// ── Poll loop ─────────────────────────────────────────────────────────────────
async function pollTorrents() {
    try {
        const res  = await fetch('/api/list');
        const list = await res.json();
        renderList(list);
    } catch (e) {
        setStatus('Connection error – retrying…', true);
    }
    pollTimer = setTimeout(pollTorrents, 2000);
}

// ── Render ────────────────────────────────────────────────────────────────────
function renderList(list) {
    const tbody = document.getElementById('torrentBody');
    const empty = document.getElementById('emptyRow');

    const seen = new Set();

    for (const t of list) {
        seen.add(t.hash);
        let row = document.getElementById('tr_' + t.hash);
        if (!row) {
            row = createRow(t.hash);
            tbody.appendChild(row);
            if (empty) empty.style.display = 'none';
        }
        updateRow(row, t);
    }

    // Remove rows for torrents no longer reported
    Array.from(tbody.querySelectorAll('tr[id^="tr_"]')).forEach(row => {
        const h = row.id.slice(3);
        if (!seen.has(h)) row.remove();
    });

    if (list.length === 0 && empty) {
        empty.style.display = '';
    }
}

function createRow(hash) {
    const row = document.createElement('tr');
    row.id = 'tr_' + hash;
    row.innerHTML = `
      <td class="col-name"><span class="t-name"></span></td>
      <td class="col-size t-size" style="text-align:right"></td>
      <td class="col-progress">
        <div class="progress-wrap"><div class="progress-fill t-bar"></div></div>
        <div class="progress-label t-pct"></div>
      </td>
      <td class="col-dl t-dl" style="text-align:right"></td>
      <td class="col-ul t-ul" style="text-align:right"></td>
      <td class="col-peers t-peers" style="text-align:center"></td>
      <td class="col-state" style="text-align:center"><span class="badge t-badge"></span></td>
      <td class="col-actions" style="text-align:center">
        <button class="action-btn action-start"  onclick="torrentStart('${hash}')">&#x25B6;</button>
        <button class="action-btn action-stop"   onclick="torrentStop('${hash}')">&#x23F8;</button>
        <button class="action-btn action-remove" onclick="openRemoveDialog('${hash}','')">&#x1F5D1;</button>
      </td>`;
    return row;
}

function updateRow(row, t) {
    const pct  = Math.round(t.progress * 100);
    const cls  = t.state;   // downloading | seeding | stopped | checking | metadata

    row.querySelector('.t-name').textContent  = t.name || '(loading metadata…)';
    row.querySelector('.t-size').textContent  = t.totalSize ? fmtBytes(t.totalSize) : '—';
    row.querySelector('.t-pct').textContent   = pct + '%';
    row.querySelector('.t-dl').textContent    = fmtSpeed(t.downloadSpeed);
    row.querySelector('.t-ul').textContent    = fmtSpeed(t.uploadSpeed);
    row.querySelector('.t-peers').textContent = t.peers != null ? t.peers : '—';

    const bar = row.querySelector('.t-bar');
    bar.style.width     = pct + '%';
    bar.className       = 'progress-fill ' + cls;

    const badge = row.querySelector('.t-badge');
    badge.textContent   = cls;
    badge.className     = 'badge ' + cls;

    // Update the Remove dialog button label reference
    const removeBtn = row.querySelector('.action-remove');
    if (removeBtn) {
        removeBtn.setAttribute('onclick',
            `openRemoveDialog('${t.hash}','${(t.name||'').replace(/'/g,"\\'")}')`)
    }
}

// ── Add Magnet dialog ─────────────────────────────────────────────────────────
function openMagnetDialog() {
    document.getElementById('magnetInput').value = '';
    document.getElementById('magnetOverlay').style.display = 'flex';
    document.getElementById('magnetInput').focus();
}

function closeMagnetDialog() {
    document.getElementById('magnetOverlay').style.display = 'none';
}

async function submitMagnet() {
    const magnet = document.getElementById('magnetInput').value.trim();
    if (!magnet) return;
    closeMagnetDialog();
    setStatus('Adding magnet…');
    try {
        const data = await apiPost('/api/add_magnet', { magnet });
        if (data.ok) setStatus('Torrent added (' + (data.hash||'').slice(0,8) + '…)');
        else         setStatus('Error: ' + (data.error || 'unknown'), true);
    } catch (e) {
        setStatus('Network error', true);
    }
}

// ── Upload .torrent file ──────────────────────────────────────────────────────
async function uploadTorrentFile(input) {
    const file = input.files[0];
    input.value = '';
    if (!file) return;
    setStatus('Uploading ' + file.name + '…');
    try {
        const buf = await file.arrayBuffer();
        const res = await fetch('/api/add_torrent', {
            method: 'POST',
            headers: { 'Content-Type': 'application/octet-stream' },
            body:   buf
        });
        const data = await res.json();
        if (data.ok) setStatus('Added: ' + (data.name || data.hash));
        else         setStatus('Error: ' + (data.error || 'unknown'), true);
    } catch (e) {
        setStatus('Upload failed', true);
    }
}

// ── Torrent controls ──────────────────────────────────────────────────────────
async function torrentStart(hash) {
    setStatus('Starting…');
    const data = await apiPost('/api/start', { hash });
    setStatus(data.ok ? '' : 'Error: ' + data.error, !data.ok);
}

async function torrentStop(hash) {
    setStatus('Stopping…');
    const data = await apiPost('/api/stop', { hash });
    setStatus(data.ok ? '' : 'Error: ' + data.error, !data.ok);
}

// ── Remove dialog ─────────────────────────────────────────────────────────────
function openRemoveDialog(hash, name) {
    removeHash  = hash;
    removeName_ = name;
    document.getElementById('removeName').textContent =
        'Are you sure you want to remove "' + (name || hash.slice(0,8) + '…') + '"?';
    document.getElementById('deleteFilesChk').checked = false;
    document.getElementById('removeOverlay').style.display = 'flex';
    document.getElementById('confirmRemoveBtn').onclick = confirmRemove;
}

function closeRemoveDialog() {
    document.getElementById('removeOverlay').style.display = 'none';
    removeHash = null;
}

async function confirmRemove() {
    if (!removeHash) return;
    closeRemoveDialog();
    const del = document.getElementById('deleteFilesChk').checked ? 1 : 0;
    const data = await apiPost('/api/remove', { hash: removeHash, delete_files: del });
    setStatus(data.ok ? 'Removed.' : 'Error: ' + data.error, !data.ok);
}

// ── Keyboard shortcuts ────────────────────────────────────────────────────────
document.addEventListener('keydown', e => {
    if (e.key === 'Escape') {
        closeMagnetDialog();
        closeRemoveDialog();
    }
});

// Allow Enter to submit the magnet dialog
document.addEventListener('DOMContentLoaded', () => {
    document.getElementById('magnetInput').addEventListener('keydown', e => {
        if (e.key === 'Enter') submitMagnet();
    });
    pollTorrents();
});
