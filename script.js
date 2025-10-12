const editor = document.getElementById('editor');
const wordCountEl = document.getElementById('wordCount');
const statusEl = document.getElementById('status');

const userId = Math.random().toString(36).substr(2, 9);
const userColor = "#" + Math.floor(Math.random()*16777215).toString(16);

let ws;
let authorized = false;

const remoteUsers = {}; // userId => { cursorEl }
const remoteSelections = {};
let lastContent = localStorage.getItem("autosave") || editor.innerHTML;

editor.innerHTML = lastContent;

// ---------------- Ask for username/password ----------------
function promptAuth() {
    const username = prompt("Enter your username:");
    const password = prompt("Enter your password:");
    return { username, password };
}

// ---------------- Connect WebSocket ----------------
function connect() {
    ws = new WebSocket("ws://localhost:9000");

    ws.onopen = () => {
        statusEl.textContent = "Status: Connecting...";
        const creds = promptAuth();
        ws.send(JSON.stringify({ type: 'auth', username: creds.username, password: creds.password }));
    };

    ws.onclose = () => {
        statusEl.textContent = "Status: Disconnected ❌ (Reconnecting...)";
        authorized = false;
        setTimeout(connect, 2000);
    };

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);

        // Handle auth response
        if (data.type === 'auth_resp') {
            if (data.status === 'ok') {
                authorized = true;
                editor.setAttribute('contenteditable', true);
                statusEl.textContent = "Status: Connected ✅";
            } else {
                authorized = false;
                editor.setAttribute('contenteditable', false);
                statusEl.textContent = "Status: Unauthorized ❌";
                alert("Authorization failed! Refresh to try again.");
            }
            return;
        }

        if (!authorized) return;
        if (data.userId === userId) return;

        if (data.type === 'update') safeUpdateContent(data.content);
        if (data.type === 'cursor') renderRemoteCursor(data.userId, data.cursor, data.color);
        if (data.type === 'selection') renderRemoteSelection(data.userId, data.startOffset, data.endOffset, data.color);
        if (data.type === 'disconnect') removeRemoteCursor(data.userId);
    };
}
connect();

// ---------------- Safe content update ----------------
function safeUpdateContent(newContent) {
    const sel = window.getSelection();
    let startOffset = 0, endOffset = 0, startNode = null, endNode = null;

    if (sel.rangeCount > 0) {
        const range = sel.getRangeAt(0);
        startOffset = range.startOffset;
        endOffset = range.endOffset;
        startNode = range.startContainer;
        endNode = range.endContainer;
    }

    if (editor.innerHTML !== newContent) editor.innerHTML = newContent;

    if (startNode && editor.contains(startNode)) {
        const range = document.createRange();
        range.setStart(startNode, Math.min(startOffset, startNode.textContent.length));
        range.setEnd(endNode, Math.min(endOffset, endNode.textContent.length));
        sel.removeAllRanges();
        sel.addRange(range);
    }

    updateWordCount();
}

// ---------------- Word count ----------------
function updateWordCount() {
    const words = editor.innerText.trim().length ? editor.innerText.trim().split(/\s+/).length : 0;
    wordCountEl.textContent = `Words: ${words}`;
}

// ---------------- Local edits ----------------
editor.addEventListener('input', () => {
    if (!authorized) return;
    lastContent = editor.innerHTML;
    if (ws.readyState === WebSocket.OPEN)
        ws.send(JSON.stringify({ type: 'update', userId, content: lastContent }));
    localStorage.setItem("autosave", lastContent);
    sendCursorUpdate();
    sendSelectionUpdate();
});

// ---------------- Formatting ----------------
function format(cmd, value=null) {
    if (!authorized) return;
    document.execCommand(cmd, false, value);
    lastContent = editor.innerHTML;
    if (ws.readyState === WebSocket.OPEN)
        ws.send(JSON.stringify({ type: 'update', userId, content: lastContent }));
    localStorage.setItem("autosave", lastContent);
    updateWordCount();
}

// ---------------- Cursor updates ----------------
function getCaretCoords() {
    const sel = window.getSelection();
    if (!sel.rangeCount) return null;
    const range = sel.getRangeAt(0).cloneRange();
    range.collapse(true);
    const rect = range.getBoundingClientRect();
    const editorRect = editor.getBoundingClientRect();
    return { x: rect.left - editorRect.left, y: rect.top - editorRect.top };
}

function sendCursorUpdate() {
    if (!authorized) return;
    const pos = getCaretCoords();
    if (!pos || ws.readyState !== WebSocket.OPEN) return;
    ws.send(JSON.stringify({ type: 'cursor', userId, color: userColor, cursor: pos }));
}

function sendSelectionUpdate() {
    if (!authorized) return;
    const sel = window.getSelection();
    if (!sel.rangeCount || ws.readyState !== WebSocket.OPEN) return;
    const range = sel.getRangeAt(0);
    const start = getOffsetInEditor(range.startContainer, range.startOffset);
    const end = getOffsetInEditor(range.endContainer, range.endOffset);
    ws.send(JSON.stringify({ type: 'selection', userId, color: userColor, startOffset: start, endOffset: end }));
}

// ---------------- Helpers ----------------
function getOffsetInEditor(node, offset) {
    let charIndex = offset;
    let current = node;
    while (current && current !== editor) {
        while (current.previousSibling) {
            current = current.previousSibling;
            charIndex += current.textContent.length;
        }
        current = current.parentNode;
    }
    return charIndex;
}

// ---------------- Remote cursors ----------------
function renderRemoteCursor(uid, pos, color) {
    let obj = remoteUsers[uid];
    if (!obj) {
        const cursorEl = document.createElement('div');
        cursorEl.className = 'remote-cursor';
        cursorEl.style.position = 'absolute';
        cursorEl.style.width = '2px';
        cursorEl.style.height = '1em';
        cursorEl.style.backgroundColor = color;
        editor.appendChild(cursorEl);
        obj = { cursorEl };
        remoteUsers[uid] = obj;
    }
    obj.cursorEl.style.left = pos.x + 'px';
    obj.cursorEl.style.top = pos.y + 'px';
}

function removeRemoteCursor(uid) {
    const obj = remoteUsers[uid];
    if (obj) editor.removeChild(obj.cursorEl);
    delete remoteUsers[uid];
}

// ---------------- Remote selection ----------------
function renderRemoteSelection(uid, start, end, color) {
    removeRemoteSelection(uid);
    const range = document.createRange();
    const sel = window.getSelection();
    sel.removeAllRanges();
    setSelectionFromOffsets(start, end);
    const span = document.createElement('span');
    span.className = 'remote-selection';
    span.style.backgroundColor = color + '55';
    range.surroundContents(span);
    remoteSelections[uid] = span;
    sel.removeAllRanges();
}

function removeRemoteSelection(uid) {
    const el = remoteSelections[uid];
    if (!el) return;
    const parent = el.parentNode;
    while (el.firstChild) parent.insertBefore(el.firstChild, el);
    parent.removeChild(el);
    delete remoteSelections[uid];
}

function setSelectionFromOffsets(start, end) {
    const sel = window.getSelection();
    const range = document.createRange();
    let charIndex = 0, startNode=null, startOffset=0, endNode=null, endOffset=0;

    function traverse(node) {
        if (node.nodeType === Node.TEXT_NODE) {
            const nextIndex = charIndex + node.textContent.length;
            if (!startNode && start >= charIndex && start <= nextIndex) {
                startNode = node; startOffset = start - charIndex;
            }
            if (!endNode && end >= charIndex && end <= nextIndex) {
                endNode = node; endOffset = end - charIndex;
            }
            charIndex = nextIndex;
        } else node.childNodes.forEach(traverse);
    }
    traverse(editor);

    if (startNode && endNode) {
        range.setStart(startNode, startOffset);
        range.setEnd(endNode, endOffset);
        sel.removeAllRanges();
        sel.addRange(range);
    }
}

// ---------------- Event listeners ----------------
editor.addEventListener('keyup', () => { sendCursorUpdate(); sendSelectionUpdate(); });
editor.addEventListener('mouseup', () => { sendCursorUpdate(); sendSelectionUpdate(); });

// ---------------- Disconnect ----------------
window.addEventListener("beforeunload", () => {
    if (!authorized) return;
    if (ws.readyState === WebSocket.OPEN)
        ws.send(JSON.stringify({ type: 'disconnect', userId }));
});
