import React, { useState, useEffect, useRef } from 'react';
import { io, Socket } from 'socket.io-client';
// @ts-ignore
import JMuxer from 'jmuxer';
import win32Translations from './translations.json';

const SERVER_URL = import.meta.env.PROD ? undefined : 'http://localhost:5000';

interface User {
  userId: string;
  userName: string;
  nclr: number;
}

interface ChatMessage {
  id: string;
  userId: string;
  userName: string;
  msg: string;
  clr: number;
  nclr?: number;
  b?: boolean;
  i?: boolean;
  u?: boolean;
  sz?: number;
  isSystem?: boolean;
  systemColor?: string;
}

// Win32 typical color palette approximation for nclr
const colors = [
  '#000000', // 0: Black
  '#0000A0', // 1: Dark Blue
  '#008000', // 2: Green
  '#008080', // 3: Teal
  '#800000', // 4: Maroon
  '#800080', // 5: Purple
  '#808000', // 6: Olive
  '#C0C0C0', // 7: Light Gray
  '#808080', // 8: Dark Gray
  '#0000FF', // 9: Blue
  '#00FF00', // 10: Lime
  '#00FFFF', // 11: Cyan
  '#FF0000', // 12: Red
  '#FF00FF', // 13: Magenta
  '#FFFF00', // 14: Yellow
  '#FFFFFF', // 15: White
  '#FF8C00'  // 16: Orange (for user_573 in screenshot)
];

const extendedColors = [
  '#FF8080', '#FFFF80', '#80FF80', '#00FF80', '#80FFFF', '#0080FF', '#FF80C0', '#FF80FF',
  '#FF0000', '#FFFF00', '#80FF00', '#00FF40', '#00FFFF', '#0080C0', '#8080C0', '#FF00FF',
  '#C08080', '#C0C000', '#00FF00', '#00C000', '#00C0C0', '#0000FF', '#0000A0', '#800080',
  '#800000', '#808000', '#008000', '#008040', '#008080', '#000080', '#400040', '#400080',
  '#400000', '#404000', '#004000', '#004040', '#004080', '#000040', '#200020', '#200040',
  '#FFFFFF', '#E0E0E0', '#C0C0C0', '#808080', '#404040', '#000000', '#000000', '#000000'
];

const getColor = (val: number) => {
  if (val >= 0 && val < colors.length) {
    return colors[val];
  }
  // Convert Win32 COLORREF (0x00bbggrr) to RGB
  const r = val & 0xFF;
  const g = (val >> 8) & 0xFF;
  const b = (val >> 16) & 0xFF;
  return `rgb(${r}, ${g}, ${b})`;
};

const toColorRef = (val: number) => {
  if (val >= 0 && val < colors.length) {
    const hex = colors[val];
    const r = parseInt(hex.substring(1, 3), 16);
    const g = parseInt(hex.substring(3, 5), 16);
    const b = parseInt(hex.substring(5, 7), 16);
    return r | (g << 8) | (b << 16);
  }
  return val;
};

const getHexColor = (val: number) => {
  const ref = toColorRef(val);
  const r = ref & 0xFF;
  const g = (ref >> 8) & 0xFF;
  const b = (ref >> 16) & 0xFF;
  return `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`;
};

const toColorRefFromHex = (hex: string) => {
  const r = parseInt(hex.substring(1, 3), 16);
  const g = parseInt(hex.substring(3, 5), 16);
  const b = parseInt(hex.substring(5, 7), 16);
  return r | (g << 8) | (b << 16);
};

const renderMessageWithLinks = (text: string) => {
  const urlRegex = /(https?:\/\/[a-zA-Z0-9\-._~:/?#[\]@!$&'()*+,;=%]+)/g;
  const parts = text.split(urlRegex);
  return parts.map((part, i) => {
    if (part.match(urlRegex)) {
      return (
        <a key={i} href={part} target="_blank" rel="noopener noreferrer" style={{ color: 'blue', textDecoration: 'underline' }}>
          {part}
        </a>
      );
    }
    return part;
  });
};

const urlParams = new URLSearchParams(window.location.search);
let langParam = (urlParams.get('lang') || 'ja').toLowerCase();

const langNameMap: Record<string, string> = {
  arabic: 'ar', bengali: 'bn', indonesian: 'id', italian: 'it',
  turkish: 'tr', vietnamese: 'vi', english: 'en', japanese: 'ja',
  chinese: 'zh-cn', korean: 'ko', french: 'fr', german: 'de',
  spanish: 'es', hindi: 'hi', portuguese: 'pt', russian: 'ru'
};
if (langNameMap[langParam]) {
  langParam = langNameMap[langParam];
}

const dict = {
  ja: {
    joinedNetwork: (name: string) => `ネットワークに参加しました (${name})`,
    movingTo: (room: string) => `${room} に移動しています...`,
    enteredRoom: (room: string) => `${room} に入室しました。`,
    userEntered: (user: string, room: string) => `${user}が${room}に入室しました。`,
    userLeft: (user: string, room: string) => `${user}が${room}から退室しました。`,
    send: '送信',
    watchVideo: '映像を見る',
    stopVideo: '映像を停止',
    mic: '話す',
    camera: 'カメラ',
    screen: '画面共有',
    moveRoom: '部屋移動...',
    moveRoomBtn: '移動',
    changeName: '名前変更...',
    systemSettings: 'システム設定',
    disconnect: '切断',
    close: '閉じる',
    move: '移動する',
    cancel: 'キャンセル',
    change: '変更する',
    receivingVideo: '映像受信中...',
    loading: '読込中...',
    noCameraAccess: 'Webカメラにアクセスできませんでした。',
    noMicAccess: 'マイクにアクセスできませんでした。',
    noScreenAccess: '画面共有を取得できませんでした。',
    roomName: '部屋名',
    name: '名前',
    color: '文字色',
    noticeJa: '※このページは日本語で表示しています。',
    users: 'ユーザー一覧',
    self: '自分',
    stopCamera: '映像配信を停止する',
    startCamera: '映像を配信する',
    shareScreen: '画面共有',
    notImplemented: '無視機能は未実装です',
    ignore: '無視する',
    reported: '通報しました',
    report: '通報する',
    publicRooms: '公開部屋一覧',
    people: '人',
    selectColor: '色の選択',
    custom: 'カスタム',
    lobby: 'ロビー',
    privateRoom: '非公開'
  },
  en: {
    joinedNetwork: (name: string) => `Joined network (${name})`,
    movingTo: (room: string) => `Moving to ${room}...`,
    enteredRoom: (room: string) => `Entered ${room}.`,
    userEntered: (user: string, room: string) => `${user} entered ${room}.`,
    userLeft: (user: string, room: string) => `${user} left ${room}.`,
    send: 'Send',
    watchVideo: 'Watch Video',
    stopVideo: 'Stop Video',
    mic: 'Talk',
    camera: 'Camera',
    screen: 'Screen',
    moveRoom: 'Move Room...',
    moveRoomBtn: 'Move Room',
    changeName: 'Change Name...',
    systemSettings: 'System Settings',
    disconnect: 'Disconnect',
    close: 'Close',
    move: 'Move',
    cancel: 'Cancel',
    change: 'Change',
    receivingVideo: 'Receiving video...',
    loading: 'Loading...',
    noCameraAccess: 'Could not access webcam.',
    noMicAccess: 'Could not access microphone.',
    noScreenAccess: 'Could not access screen sharing.',
    roomName: 'Room Name',
    name: 'Name',
    color: 'Text Color',
    noticeJa: '',
    users: 'Users',
    self: 'Self',
    stopCamera: 'Stop Camera',
    startCamera: 'Start Camera',
    shareScreen: 'Share Screen',
    notImplemented: 'Not implemented',
    ignore: 'Ignore',
    reported: 'Reported',
    report: 'Report',
    publicRooms: 'Public Rooms',
    people: '',
    selectColor: 'Select Color',
    custom: 'Custom',
    lobby: 'Lobby',
    privateRoom: 'Private'
  }
};

const win32Dict = (win32Translations as any)[langParam] || (win32Translations as any)[langParam.split('-')[0]] || (win32Translations as any)['en'] || {};
const baseDict = langParam === 'ja' ? dict.ja : dict.en;

const featureFlags = {
  room: urlParams.get('room') || win32Dict.lobby || baseDict.lobby || 'ロビー',
  showUsers: urlParams.get('showUsers') !== '0' && urlParams.get('showUsers') !== 'false',
  showMoveRoom: urlParams.get('showMoveRoom') !== '0' && urlParams.get('showMoveRoom') !== 'false',
  showSystemMessages: urlParams.get('showSystemMessages') !== '0' && urlParams.get('showSystemMessages') !== 'false',
  adminToken: urlParams.get('admin') || '',
  enableCamera: urlParams.get('enableCamera') !== '0' && urlParams.get('enableCamera') !== 'false',
  enableScreen: urlParams.get('enableScreen') !== '0' && urlParams.get('enableScreen') !== 'false',
  enableMic: urlParams.get('enableMic') !== '0' && urlParams.get('enableMic') !== 'false',
  enableAudio: urlParams.get('enableAudio') !== '0' && urlParams.get('enableAudio') !== 'false',
  enableVideo: urlParams.get('enableVideo') !== '0' && urlParams.get('enableVideo') !== 'false',
  showFrame: urlParams.get('showFrame') !== '0' && urlParams.get('showFrame') !== 'false',
  isPrivate: urlParams.get('private') === '1' || urlParams.get('private') === 'true',
};

// Update document language dynamically
document.documentElement.lang = langParam;

if (!featureFlags.showFrame) {
  document.body.style.backgroundColor = '#c0c0c0';
  document.body.style.margin = '0';
  document.body.style.padding = '0';
  const style = document.createElement('style');
  style.innerHTML = `
    #root { width: 100%; height: 100%; display: flex; flex-direction: column; }
    .window.no-frame { width: 100%; height: 100%; border: none !important; box-shadow: none !important; }
    .window.no-frame .window-body { height: 100% !important; margin: 0; padding: 0 !important; }
  `;
  document.head.appendChild(style);
}

const t = {
  ...baseDict,
  ...win32Dict,
  changeName: win32Dict.changeNameTitle || baseDict.changeName,
  moveRoom: win32Dict.moveRoomTitle || baseDict.moveRoom,
  moveRoomBtn: win32Dict.moveRoomBtn || baseDict.moveRoomBtn,
  watchVideo: win32Dict.watchVideo || baseDict.watchVideo,
  ignore: win32Dict.ignore || baseDict.ignore,
  talk: win32Dict.talk || baseDict.mic,
  mic: win32Dict.talk || baseDict.mic,
  send: win32Dict.send || baseDict.send,
  self: win32Dict.self || baseDict.self,
  report: win32Dict.report || baseDict.report,
  reported: win32Dict.reported || baseDict.reported,
  cancel: win32Dict.cancel || baseDict.cancel,
  move: win32Dict.move || baseDict.move,
  change: win32Dict.change || baseDict.change,
  joinedNetwork: (name: string) => win32Dict.joined ? `${win32Dict.joined} (${name})` : baseDict.joinedNetwork(name),
  movingTo: (room: string) => win32Dict.movedTo ? win32Dict.movedTo.replace('%s', room) : baseDict.movingTo(room),
  userEntered: (user: string, room: string) => win32Dict.entered ? `${user}${win32Dict.entered}` : baseDict.userEntered(user, room),
  userLeft: (user: string, room: string) => win32Dict.left ? `${user}${win32Dict.left}` : baseDict.userLeft(user, room),
  enteredRoom: (room: string) => win32Dict.entered ? `${win32Dict.entered.trim()}` : baseDict.enteredRoom(room),
};

const MicIcon = () => {
  return (
    <svg width="14" height="16" viewBox="0 0 14 16" xmlns="http://www.w3.org/2000/svg" style={{ display: 'block' }}>
      {/* Base Icon */}
      <g stroke="#282828" fill="#282828">
        <rect x="5" y="2" width="4" height="6" />
        <circle cx="7" cy="2" r="2" />
        <circle cx="7" cy="8" r="2" />
        <path d="M3 8 A4 4 0 0 0 11 8" strokeWidth="1.5" fill="none" strokeLinecap="square" />
        <line x1="7" y1="12" x2="7" y2="15" strokeWidth="1.5" />
        <line x1="4" y1="15" x2="10" y2="15" strokeWidth="1.5" />
      </g>
    </svg>
  );
};

const getSecretId = () => {
  let id = localStorage.getItem('chat_secret_id');
  if (!id) {
    id = crypto.randomUUID();
    localStorage.setItem('chat_secret_id', id);
  }
  return id;
};

const safeColors = [
  '#C83232', '#329632', '#3232C8',
  '#B4B432', '#B432B4', '#32B4B4',
  '#FF8000', '#8000FF', '#0080FF',
  '#80C800', '#FF0080', '#00C880'
];

const userViolations: Record<string, number> = {};
const userMessageTimes: Record<string, number[]> = {};

function sanitizeText(str: string, maxLength: number = 1000): string {
  if (!str) return "";
  return str.replace(/[\u0300-\u036F\u1AB0-\u1AFF\u1DC0-\u1DFF\u20D0-\u20FF\uFE20-\uFE2F]/g, '').substring(0, maxLength);
}

function filterProfanity(text: string, userId: string, onIgnore: (uid: string) => void): string {
  if (!text) return text;
  
  const level1_regex = /(死|氏|し)(ね|んじまえ|ばいい)|(殺|56)(す|して)|キチ[ガガイ〇]イ|池沼|土人|チャンコロ|カタワ|メクラ|援助交際|パパ活|売春|強姦|自殺しろ|\b(fuck|bitch|cunt|nigger|nigga|faggot)\b|操你妈|傻逼|干你娘|肏你妈|씨발|개새끼|지랄|병신/i;
  
  if (level1_regex.test(text)) {
    onIgnore(userId);
    return "";
  }
  
  const level2_regex = /まんこ|マンコ|おめこ|ペニス|ちんこ|ウンコ|糞|マヌケ|キモイ|ウザい|\b(shit|slut|whore|asshole|dick|cock|pussy)\b|靠北|智障|脑残|鸡巴|牛逼|좆|병신|씨발|년/gi;
  
  const filteredText = text.replace(level2_regex, "***");
  if (filteredText !== text) {
    userViolations[userId] = (userViolations[userId] || 0) + 1;
    if (userViolations[userId] >= 3) {
      onIgnore(userId);
    }
  }
  
  return filteredText;
}

const App: React.FC = () => {
  const [socket, setSocket] = useState<Socket | null>(null);
  const [localUserId, setLocalUserId] = useState<string>('');
  const localUserIdRef = useRef<string>('');
  
  // Use lazy initialization so these are only generated once or read from storage
  const [name, setName] = useState(() => {
    const savedName = localStorage.getItem('chat_user_name');
    if (savedName) return sanitizeText(savedName, 50);
    const newName = `User_${Math.floor(Math.random() * 900) + 100}`;
    localStorage.setItem('chat_user_name', newName);
    return newName;
  });

  const localUserNameRef = useRef<string>(name);

  useEffect(() => {
    localUserNameRef.current = name;
  }, [name]);

  const [nclr, setNclr] = useState(() => {
    const savedColor = localStorage.getItem('chat_user_color');
    if (savedColor !== null) return parseInt(savedColor, 10);
    const hexColor = safeColors[Math.floor(Math.random() * safeColors.length)];
    const newColor = toColorRefFromHex(hexColor);
    localStorage.setItem('chat_user_color', newColor.toString());
    return newColor;
  });
  const [msgColor, setMsgColor] = useState(0); // Default to black
  const [ignoredUsers, setIgnoredUsers] = useState<string[]>(() => {
    try {
      const stored = localStorage.getItem('chat_ignored_users');
      return stored ? JSON.parse(stored) : [];
    } catch {
      return [];
    }
  });
  const ignoredUsersRef = useRef<string[]>(ignoredUsers);
  useEffect(() => {
    ignoredUsersRef.current = ignoredUsers;
    localStorage.setItem('chat_ignored_users', JSON.stringify(ignoredUsers));
  }, [ignoredUsers]);

  const [isPrivateRoom, setIsPrivateRoom] = useState(false);
  const [roomId, setRoomId] = useState(featureFlags.room);
  
  useEffect(() => {
    document.title = `chat.exe - [${roomId}]${isPrivateRoom ? ' (非公開)' : ''}`;
  }, [roomId, isPrivateRoom]);
  
  const [users, setUsers] = useState<User[]>([]);
  const usersRef = useRef<User[]>(users);
  useEffect(() => {
    usersRef.current = users;
  }, [users]);
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [messageInput, setMessageInput] = useState('');
  const chatInputRef = useRef<HTMLTextAreaElement>(null);
  const [contextMenu, setContextMenu] = useState<{ x: number, y: number, userId: string, isSelf: boolean } | null>(null);
  const [selectedUserId, setSelectedUserId] = useState<string | null>(null);

  const [showColorPicker, setShowColorPicker] = useState(false);
  const [colorPickerTarget, setColorPickerTarget] = useState<'msg' | 'name' | null>(null);
  const [colorPickerPos, setColorPickerPos] = useState({ x: 0, y: 0 });
  const [tempMsgColor, setTempMsgColor] = useState(0);

  const [streamingUsers, setStreamingUsers] = useState<Record<string, number>>({});
  const [watchingUsers, setWatchingUsers] = useState<Record<string, boolean>>({});
  const [videoPositions, setVideoPositions] = useState<Record<string, {x: number, y: number}>>({});

  // Formatting states
  const [isBold, setIsBold] = useState(false);
  const [isItalic, setIsItalic] = useState(false);
  const [isUnderline, setIsUnderline] = useState(false);
  const [fontSize, setFontSize] = useState(10);

  // Room move dialog state
  const [showMoveRoom, setShowMoveRoom] = useState(false);
  const [newRoomName, setNewRoomName] = useState('');
  const [publicRooms, setPublicRooms] = useState<{name: string, count: number}[]>([]);

  // Name change dialog state
  const [showChangeName, setShowChangeName] = useState(false);
  const [newNameInput, setNewNameInput] = useState('');
  const [newColorInput, setNewColorInput] = useState(0);
  const [viewerCounts, setViewerCounts] = useState<{ [userId: string]: number }>({});

  // Audio state
  const [isMicActive, setIsMicActive] = useState(false);
  const audioContextRef = useRef<AudioContext | null>(null);
  const masterGainRef = useRef<GainNode | null>(null);
  const [volume, setVolume] = useState<number>(50);
  const mediaStreamRef = useRef<MediaStream | null>(null);
  const scriptProcessorRef = useRef<ScriptProcessorNode | null>(null);
  const [micLevel, setMicLevel] = useState(0);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const rafRef = useRef<number>(0);

  // Video state
  const [isWebcamActive, setIsWebcamActive] = useState(false);
  const [isScreenActive, setIsScreenActive] = useState(false);
  const [showSelfPreview, setShowSelfPreview] = useState(false);
  const selfVideoRef = useRef<HTMLVideoElement>(null);
  const videoStreamRef = useRef<MediaStream | null>(null);
  const videoIntervalRef = useRef<number | null>(null);
  const videoCanvasRef = useRef<HTMLCanvasElement | null>(null);
  const videoElRef = useRef<HTMLVideoElement | null>(null);
  // Audio playback queue
  const nextPlayTimeRef = useRef<Record<string, number>>({});
  // Incoming Video State (Map of userId -> image URL or canvas ref)
  const [videoFrames, setVideoFrames] = useState<Record<string, string>>({});
  const [talkingUsers, setTalkingUsers] = useState<Record<string, number>>({});
  const jmuxersRef = useRef<Map<string, any>>(new Map());

  // Audio playback queue

  const updateMicLevel = () => {
    if (!analyserRef.current) return;
    const dataArray = new Uint8Array(analyserRef.current.frequencyBinCount);
    analyserRef.current.getByteFrequencyData(dataArray);
    let sum = 0;
    for (let i = 0; i < dataArray.length; i++) {
      sum += dataArray[i];
    }
    const average = sum / dataArray.length;
    setMicLevel(Math.min(100, Math.floor((average / 128) * 100)));
    rafRef.current = requestAnimationFrame(updateMicLevel);
  };

  const initAudio = () => {
    if (!audioContextRef.current) {
      const audioCtx = new (window.AudioContext || (window as any).webkitAudioContext)({ sampleRate: 16000 });
      audioContextRef.current = audioCtx;
      
      const compressor = audioCtx.createDynamicsCompressor();
      compressor.threshold.value = -6;
      compressor.knee.value = 12;
      compressor.ratio.value = 12;
      compressor.attack.value = 0.003;
      compressor.release.value = 0.25;
      compressor.connect(audioCtx.destination);

      const gainNode = audioCtx.createGain();
      gainNode.connect(compressor);
      gainNode.gain.value = volume / 100;
      masterGainRef.current = gainNode;
    } else if (audioContextRef.current.state === 'suspended') {
      audioContextRef.current.resume();
    }
  };

  useEffect(() => {
    if (masterGainRef.current) {
      masterGainRef.current.gain.value = volume / 100;
    }
  }, [volume]);

  const chatLogRef = useRef<HTMLDivElement>(null);

  // Auto-close dropdowns
  useEffect(() => {
    const handleClick = () => {
      setContextMenu(null);
      setSelectedUserId(null);
    };
    window.addEventListener('click', handleClick);
    return () => window.removeEventListener('click', handleClick);
  }, []);

  const [isJoined, setIsJoined] = useState(false);

  const connectToRoom = async (targetRoom: string, targetName?: string, targetColor?: number, isPrivate = false) => {
    initAudio();
    const finalName = sanitizeText(targetName || name, 50);
    const finalColor = targetColor !== undefined ? targetColor : nclr;
    
    setIsJoined(false);
    setIsPrivateRoom(isPrivate);

    if (socket) {
      socket.emit('leave', { userId: localUserIdRef.current });
      socket.disconnect();
    }
    
    setRoomId(targetRoom);
    if (featureFlags.showSystemMessages) {
      setMessages(prev => [
        ...prev,
        { id: crypto.randomUUID(), userId: '', userName: '', msg: t.joinedNetwork(finalName.split('#')[0]), clr: 0, sz: 9, isSystem: true, systemColor: '#808080' },
        { id: crypto.randomUUID(), userId: '', userName: '', msg: t.movingTo(targetRoom), clr: 0, sz: 9, isSystem: true, systemColor: '#808080' }
      ]);
    }
    setUsers([]);
    
    const newSocket = io(SERVER_URL);
    setSocket(newSocket);
    
    newSocket.on('connect', () => {
      newSocket.emit('join', {
        secretId: getSecretId(),
        userName: finalName,
        roomName: targetRoom,
        nclr: toColorRef(finalColor),
        pub: !isPrivate,
        hashRoomName: isPrivate
      });
    });
  };

  // Initialize and Auto-Join on mount
  useEffect(() => {
    connectToRoom(featureFlags.room, undefined, undefined, featureFlags.isPrivate);
    chatInputRef.current?.focus();
    // We intentionally only run this on mount, so no dependencies
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Unlock audio context on any user interaction (Browser Autoplay Policy)
  useEffect(() => {
    const unlockAudio = () => {
      if (audioContextRef.current?.state === 'suspended') {
        audioContextRef.current.resume();
      }
    };
    document.addEventListener('click', unlockAudio, { passive: true });
    document.addEventListener('keydown', unlockAudio, { passive: true });
    return () => {
      document.removeEventListener('click', unlockAudio);
      document.removeEventListener('keydown', unlockAudio);
    };
  }, []);

  // Sync current room and settings to URL
  useEffect(() => {
    const params = new URLSearchParams(window.location.search);
    params.set('room', roomId);
    params.set('private', isPrivateRoom ? '1' : '0');
    params.set('showUsers', featureFlags.showUsers ? '1' : '0');
    params.set('showMoveRoom', featureFlags.showMoveRoom ? '1' : '0');
    params.set('showSystemMessages', featureFlags.showSystemMessages ? '1' : '0');
    params.set('enableCamera', featureFlags.enableCamera ? '1' : '0');
    params.set('enableScreen', featureFlags.enableScreen ? '1' : '0');
    params.set('enableMic', featureFlags.enableMic ? '1' : '0');
    params.set('enableAudio', featureFlags.enableAudio ? '1' : '0');
    params.set('enableVideo', featureFlags.enableVideo ? '1' : '0');
    params.set('showFrame', featureFlags.showFrame ? '1' : '0');
    
    const newUrl = `${window.location.pathname}?${params.toString()}`;
    window.history.replaceState(null, '', newUrl);
  }, [roomId, isPrivateRoom]);

  // Setup Socket Events
  useEffect(() => {
    if (!socket) return;

    // Banned event
    socket.on('banned', () => {
      alert('You have been banned from the chat server.');
      setIsJoined(false);
      socket.disconnect();
    });

    socket.on('disconnect', (reason) => {
      let msg = '';
      if (reason === 'ping timeout') {
        msg = '一定時間通信がなかったため、サーバーから切断されました。';
      } else if (reason === 'transport close') {
        msg = '通信が途切れたため、サーバーから切断されました。';
      } else if (reason === 'io server disconnect') {
        msg = 'サーバーによって強制的に切断されました。（サーバー再起動などの可能性があります）';
      } else {
        msg = '予期せぬエラーにより、サーバーから切断されました。';
      }
      
      if (featureFlags.showSystemMessages) {
        setMessages(prev => [
          ...prev,
          { id: crypto.randomUUID(), userId: '', userName: '', msg, clr: 0, sz: 9, isSystem: true, systemColor: '#ff0000' }
        ]);
      }
    });

    socket.on('welcome', (data: any) => {
      if (data.userId) {
        setLocalUserId(data.userId);
        localUserIdRef.current = data.userId;
        socket.emit('get_users');
        
        setIsJoined(true);
        if (featureFlags.showSystemMessages) {
          setMessages(prev => [
            ...prev, 
            { id: crypto.randomUUID(), userId: '', userName: '', msg: t.userEntered(localUserNameRef.current.split('#')[0], roomId), clr: 0, sz: 9, isSystem: true, systemColor: '#808080' }
          ]);
        }
      }
    });

    socket.on('stream-status', (data: { userId: string }) => {
      setStreamingUsers(prev => ({ ...prev, [data.userId]: Date.now() }));
    });

    socket.on('room_users', (data: { users: User[] }) => {
      if (data.users) {
        setUsers(data.users.map(u => ({ ...u, userName: sanitizeText(u.userName, 50) })));
      }
    });

    socket.on('users', (data: any) => {
      if (data.users) {
        setUsers(data.users.map((u: User) => ({ ...u, userName: sanitizeText(u.userName, 50) })));
      }
    });

    socket.on('join', (user: User) => {
      const sanitizedUser = { ...user, userName: sanitizeText(user.userName, 50) };
      setUsers(prev => {
        if (!prev.find(u => u.userId === sanitizedUser.userId)) {
          return [...prev, sanitizedUser];
        }
        return prev;
      });
      if (sanitizedUser.userId !== localUserIdRef.current && featureFlags.showSystemMessages) {
        setMessages(prev => [
          ...prev,
          { id: crypto.randomUUID(), userId: '', userName: '', msg: t.userEntered(sanitizedUser.userName.split('#')[0], roomId), clr: 0, sz: 9, isSystem: true, systemColor: '#808080' }
        ]);
      }
    });

    socket.on('leave', (data: any) => {
      setUsers(prev => {
        const u = prev.find(u => u.userId === data.userId);
        if (u && u.userId !== localUserIdRef.current && featureFlags.showSystemMessages) {
          setMessages(msgs => [
            ...msgs,
            { id: crypto.randomUUID(), userId: '', userName: '', msg: t.userLeft(u.userName.split('#')[0], roomId), clr: 0, sz: 9, isSystem: true, systemColor: '#808080' }
          ]);
        }
        return prev.filter(user => user.userId !== data.userId);
      });
    });

    socket.on('rename', (data: { userId: string; userName: string; nclr: number }) => {
      setUsers(prev => {
        const next = [...prev];
        const idx = next.findIndex(u => u.userId === data.userId);
        if (idx !== -1) {
          next[idx] = { ...next[idx], userName: sanitizeText(data.userName, 50), nclr: data.nclr };
        }
        return next;
      });
    });

    socket.on('viewer-count', (data: { userId: string, count: number }) => {
      setViewerCounts(prev => ({ ...prev, [data.userId]: data.count }));
    });

    socket.on('chat', (data: any) => {
      const senderUserId = data.userId || 'sys';
      if (ignoredUsersRef.current.includes(senderUserId)) return;
      
      let finalMsg = data.msg;
      if (senderUserId !== 'sys' && senderUserId !== localUserIdRef.current) {
        const now = Date.now();
        const times = userMessageTimes[senderUserId] || [];
        times.push(now);
        while (times.length > 0 && now - times[0] > 3000) {
          times.shift();
        }
        userMessageTimes[senderUserId] = times;
        
        if (times.length >= 5) {
          setIgnoredUsers(prev => {
            if (prev.includes(senderUserId)) return prev;
            return [...prev, senderUserId];
          });
          return;
        }

        finalMsg = filterProfanity(data.msg, senderUserId, (uid) => {
          setIgnoredUsers(prev => {
            if (prev.includes(uid)) return prev;
            return [...prev, uid];
          });
        });
      }
      
      if (finalMsg === "") return;

      setMessages(prev => [...prev, {
        id: crypto.randomUUID(),
        userId: senderUserId,
        userName: sanitizeText(data.userName || 'System', 50),
        msg: sanitizeText(finalMsg, 1000),
        clr: data.clr !== undefined ? data.clr : 0,
        nclr: data.nclr !== undefined ? data.nclr : 0,
        b: data.b,
        i: data.i,
        u: data.u,
        sz: data.sz
      }]);
    });

    socket.on('audio-receive', (buf: ArrayBuffer) => {
      const textDecoder = new TextDecoder('utf8');
      const senderUserId = textDecoder.decode(new Uint8Array(buf, 0, 80)).replace(/\0/g, '');
      if (ignoredUsersRef.current.includes(senderUserId)) return;
      setTalkingUsers(prev => ({ ...prev, [senderUserId]: Date.now() }));
      
      const pcmByteLength = buf.byteLength - 80;
      if (pcmByteLength <= 0) return;
      
      const int16Array = new Int16Array(buf, 80, pcmByteLength / 2);
      const float32Array = new Float32Array(int16Array.length);
      for (let i = 0; i < int16Array.length; i++) {
        float32Array[i] = int16Array[i] / 32768.0;
      }
      
      if (!audioContextRef.current) return;
      const audioCtx = audioContextRef.current;
      const audioBuffer = audioCtx.createBuffer(1, float32Array.length, 16000);
      audioBuffer.copyToChannel(float32Array, 0);
      
      const source = audioCtx.createBufferSource();
      source.buffer = audioBuffer;
      if (masterGainRef.current) {
        source.connect(masterGainRef.current);
      } else {
        source.connect(audioCtx.destination);
      }
      
      let startTime = nextPlayTimeRef.current[senderUserId] || 0;
      const currentLatency = startTime - audioCtx.currentTime;
      
      if (currentLatency < 0.05) {
          // Underrun! Re-buffer 150ms
          startTime = audioCtx.currentTime + 0.15;
          source.playbackRate.value = 1.0;
      } else if (currentLatency > 0.4) {
          // Overrun! Snap back to 150ms
          startTime = audioCtx.currentTime + 0.15;
          source.playbackRate.value = 1.0;
      } else if (currentLatency > 0.2) {
          // Buffer growing, speed up slightly (1% pitch increase, unnoticeable)
          source.playbackRate.value = 1.01;
      } else if (currentLatency < 0.1) {
          // Buffer shrinking, slow down slightly (1% pitch decrease)
          source.playbackRate.value = 0.99;
      } else {
          source.playbackRate.value = 1.0;
      }
      
      source.start(startTime);
      
      // Calculate duration factoring in playbackRate to ensure next packet is scheduled correctly!
      const actualDuration = audioBuffer.duration / source.playbackRate.value;
      nextPlayTimeRef.current[senderUserId] = startTime + actualDuration;
    });

    socket.on('video-receive-jpeg', (buf: ArrayBuffer) => {
      const pcmByteLength = buf.byteLength - 80;
      if (pcmByteLength <= 0) return;
      
      const uint8 = new Uint8Array(buf, 80, pcmByteLength);
      const userIdBuf = new Uint8Array(buf, 0, 80);
      const senderUserId = new TextDecoder('utf-8').decode(userIdBuf).replace(/\0/g, '');
      if (ignoredUsersRef.current.includes(senderUserId)) return;
      
      const blob = new Blob([uint8], { type: 'image/jpeg' });
      const url = URL.createObjectURL(blob);
      
      setVideoFrames(prev => {
        if (prev[senderUserId]) URL.revokeObjectURL(prev[senderUserId]);
        return { ...prev, [senderUserId]: url };
      });
    });

    socket.on('room_list', (data: any) => {
      if (data && data.rooms) {
        setPublicRooms(data.rooms);
      }
    });

    socket.on('video-receive-h264', (buf: ArrayBuffer) => {
      const pcmByteLength = buf.byteLength - 80;
      if (pcmByteLength <= 0) return;
      
      const uint8 = new Uint8Array(buf, 80, pcmByteLength);
      const userIdBuf = new Uint8Array(buf, 0, 80);
      const senderUserId = new TextDecoder('utf-8').decode(userIdBuf).replace(/\0/g, '');
      if (ignoredUsersRef.current.includes(senderUserId)) return;
      
      // Just mark as received so Loading... disappears and the video element mounts
      setVideoFrames(prev => {
        if (prev[senderUserId] !== 'h264') {
          return { ...prev, [senderUserId]: 'h264' };
        }
        return prev;
      });

      setTimeout(() => {
        let jmuxer = jmuxersRef.current.get(senderUserId);
        if (!jmuxer) {
          const videoEl = document.getElementById(`video-element-${senderUserId}`);
          if (!videoEl) return;
          
          jmuxer = new JMuxer({
            node: videoEl,
            mode: 'video',
            flushingTime: 0,
            fps: 30,
            debug: false
          });
          jmuxersRef.current.set(senderUserId, jmuxer);
        }
        try {
          jmuxer.feed({ video: uint8 });
        } catch (e) {
          console.error('JMuxer feed error:', e);
        }
      }, 0);
    });

    return () => {
      socket.off('welcome');
      socket.off('users');
      socket.off('join');
      socket.off('leave');
      socket.off('chat');
      socket.off('audio-receive');
      socket.off('video-receive-jpeg');
      socket.off('video-receive-h264');
      socket.off('room_list');
    };
  }, [socket]);

  // Scroll to bottom of chat
  useEffect(() => {
    if (chatLogRef.current) {
      chatLogRef.current.scrollTop = chatLogRef.current.scrollHeight;
    }
  }, [messages]);

  // Clear talking users after 1000ms of inactivity
  useEffect(() => {
    const interval = setInterval(() => {
      const now = Date.now();
      setTalkingUsers(prev => {
        const next = { ...prev };
        let changed = false;
        for (const [userId, ts] of Object.entries(next)) {
          if (now - ts > 1000) {
            delete next[userId];
            changed = true;
          }
        }
        return changed ? next : prev;
      });
    }, 500);
    return () => clearInterval(interval);
  }, []);

  // Clear streaming users after 5000ms of inactivity
  useEffect(() => {
    const interval = setInterval(() => {
      const now = Date.now();
      setStreamingUsers(prev => {
        const next = { ...prev };
        let changed = false;
        for (const [userId, ts] of Object.entries(next)) {
          if (now - ts > 5000) {
            delete next[userId];
            changed = true;
          }
        }
        return changed ? next : prev;
      });
    }, 1000);
    return () => clearInterval(interval);
  }, []);

  // Close video window automatically when a user stops streaming
  useEffect(() => {
    setWatchingUsers(prev => {
      let changed = false;
      const next = { ...prev };
      for (const userId of Object.keys(next)) {
        if (next[userId] && !streamingUsers[userId]) {
          next[userId] = false;
          changed = true;
        }
      }
      return changed ? next : prev;
    });
  }, [streamingUsers]);

  // Heartbeat for watching users
  useEffect(() => {
    if (!socket) return;
    const interval = setInterval(() => {
      socket.emit('ping');
    }, 10000);
    return () => clearInterval(interval);
  }, [socket]);

  useEffect(() => {
    if (isWebcamActive || isScreenActive) {
      setShowSelfPreview(true);
    } else {
      setShowSelfPreview(false);
    }
  }, [isWebcamActive, isScreenActive]);

  useEffect(() => {
    if (showSelfPreview && selfVideoRef.current && videoStreamRef.current) {
      selfVideoRef.current.srcObject = videoStreamRef.current;
    }
  }, [showSelfPreview, isWebcamActive, isScreenActive]);

  useEffect(() => {
    if (!socket) return;
    const interval = setInterval(() => {
      Object.keys(watchingUsers).forEach(userId => {
        if (watchingUsers[userId]) {
          socket.emit('watch-video', userId);
        }
      });
    }, 1000);
    return () => clearInterval(interval);
  }, [socket, watchingUsers]);

  // @ts-ignore
  const _openMoveRoom = () => {
    setShowMoveRoom(true);
    if (socket) socket.emit('get_rooms');
  };

  const closeMoveRoom = () => {
    setShowMoveRoom(false);
    if (socket) socket.emit('close_rooms');
  };

  const sendMessage = (e: React.FormEvent) => {
    e.preventDefault();
    initAudio();
    if (!messageInput.trim() || !socket || !isJoined) return;
    
    const sanitizedMsg = sanitizeText(messageInput, 1000);
    
    socket.emit('chat', {
      msg: sanitizedMsg,
      clr: msgColor,
      nclr: toColorRef(nclr),
      b: isBold,
      i: isItalic,
      u: isUnderline,
      sz: fontSize
    });
    
    // Local Echo: C++ server doesn't broadcast back to sender
    setMessages(prev => [...prev, {
      id: crypto.randomUUID(),
      userId: localUserIdRef.current || 'self',
      userName: name,
      msg: sanitizedMsg,
      clr: msgColor,
      nclr: nclr,
      b: isBold,
      i: isItalic,
      u: isUnderline,
      sz: fontSize
    }]);
    setMessageInput('');
    // Refocus after sending
    setTimeout(() => {
      chatInputRef.current?.focus();
    }, 0);
  };

  const outBufferRef = useRef<Float32Array>(new Float32Array(0));
  const resamplePhaseRef = useRef<number>(0);
  const handleMoveRoomSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (newRoomName.trim()) {
      await connectToRoom(newRoomName.trim(), undefined, undefined, isPrivateRoom);
      closeMoveRoom();
      setNewRoomName('');
      setTimeout(() => chatInputRef.current?.focus(), 0);
    }
  };

  const handleChangeNameSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (newNameInput.trim()) {
      const finalName = sanitizeText(newNameInput.trim(), 50);
      setName(finalName);
      setNclr(newColorInput);
      localStorage.setItem('chat_user_name', finalName);
      localStorage.setItem('chat_user_color', newColorInput.toString());
      if (socket) {
        socket.emit('rename', {
          userName: finalName,
          nclr: newColorInput
        });
      }
      setUsers(prev => {
        const next = [...prev];
        const idx = next.findIndex(u => u.userId === localUserIdRef.current);
        if (idx !== -1) {
          next[idx] = { ...next[idx], userName: finalName, nclr: newColorInput };
        }
        return next;
      });
      setShowChangeName(false);
      setNewNameInput('');
      setTimeout(() => chatInputRef.current?.focus(), 0);
    }
  };

  const toggleMic = async () => {
    initAudio();
    if (isMicActive) {
      // Stop mic
      if (mediaStreamRef.current) {
        mediaStreamRef.current.getTracks().forEach(t => t.stop());
        mediaStreamRef.current = null;
      }
      if (scriptProcessorRef.current) {
        scriptProcessorRef.current.disconnect();
        scriptProcessorRef.current = null;
      }
      if (analyserRef.current) {
        analyserRef.current.disconnect();
        analyserRef.current = null;
      }
      cancelAnimationFrame(rafRef.current);
      setMicLevel(0);
      setIsMicActive(false);
    } else {
      // Start mic
      try {
        const stream = await navigator.mediaDevices.getUserMedia({ 
          audio: {
            sampleRate: 16000,
            echoCancellation: true,
            noiseSuppression: true,
            autoGainControl: true
          } 
        });
        mediaStreamRef.current = stream;
        
        const audioCtx = audioContextRef.current!;

        const source = audioCtx.createMediaStreamSource(stream);
        
        // Anti-aliasing filter for downsampling
        const lowpass = audioCtx.createBiquadFilter();
        lowpass.type = 'lowpass';
        lowpass.frequency.value = 7500;
        
        source.connect(lowpass);
        
        const analyser = audioCtx.createAnalyser();
        analyser.fftSize = 256;
        lowpass.connect(analyser);
        analyserRef.current = analyser;
        
        rafRef.current = requestAnimationFrame(updateMicLevel);

        const processor = audioCtx.createScriptProcessor(2048, 1, 1);
        scriptProcessorRef.current = processor;
        
        lowpass.connect(processor);
        
        outBufferRef.current = new Float32Array(0);
        resamplePhaseRef.current = 0;
        
        processor.onaudioprocess = (e) => {
          if (!socket) return;
          const inputData = e.inputBuffer.getChannelData(0);
          
          // Downsample from audioCtx.sampleRate to 16000 Hz
          const inputSampleRate = audioCtx.sampleRate;
          const targetSampleRate = 16000;
          let resampledData: Float32Array;
          
          if (inputSampleRate === targetSampleRate) {
            resampledData = inputData;
          } else {
            const ratio = inputSampleRate / targetSampleRate;
            let srcIdx = resamplePhaseRef.current;
            const newLength = Math.floor((inputData.length - srcIdx) / ratio) + 1;
            resampledData = new Float32Array(newLength);
            for (let i = 0; i < newLength; i++) {
              const idx1 = Math.floor(srcIdx);
              const idx2 = Math.min(idx1 + 1, inputData.length - 1);
              const frac = srcIdx - idx1;
              resampledData[i] = inputData[idx1] * (1 - frac) + inputData[idx2] * frac;
            }
          }
          
          // Append to buffer
          const newOut = new Float32Array(outBufferRef.current.length + resampledData.length);
          newOut.set(outBufferRef.current);
          newOut.set(resampledData, outBufferRef.current.length);
          outBufferRef.current = newOut;
          
          const frameSamples = 320; // 20ms at 16000Hz
          
          while (outBufferRef.current.length >= frameSamples) {
            const frame = outBufferRef.current.subarray(0, frameSamples);
            const int16Buffer = new Int16Array(frameSamples);
            for (let i = 0; i < frameSamples; i++) {
              let s = Math.max(-1, Math.min(1, frame[i]));
              int16Buffer[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
            }
            socket.emit('audio-stream', int16Buffer.buffer);
            outBufferRef.current = new Float32Array(outBufferRef.current.subarray(frameSamples));
          }
        };
        processor.connect(audioCtx.destination); // Required for Chrome to trigger onaudioprocess
        
        setIsMicActive(true);
      } catch (err) {
        console.error('Mic error:', err);
        alert(t.noMicAccess);
      }
    }
  };

  const startVideoEncodeLoop = (stream: MediaStream) => {
    if (!videoCanvasRef.current) {
      videoCanvasRef.current = document.createElement('canvas');
      videoCanvasRef.current.width = 640;
      videoCanvasRef.current.height = 480;
    }
    if (!videoElRef.current) {
      videoElRef.current = document.createElement('video');
      videoElRef.current.autoplay = true;
      videoElRef.current.muted = true;
      videoElRef.current.playsInline = true;
    }
    
    videoElRef.current.srcObject = stream;
    videoElRef.current.play().catch(()=>{});

    videoIntervalRef.current = window.setInterval(() => {
      if (videoElRef.current && videoCanvasRef.current && socket) {
        const ctx = videoCanvasRef.current.getContext('2d');
        if (ctx) {
          ctx.drawImage(videoElRef.current, 0, 0, videoCanvasRef.current.width, videoCanvasRef.current.height);
          videoCanvasRef.current.toBlob((blob) => {
            if (blob) {
              blob.arrayBuffer().then(buf => {
                socket.emit('video-stream', buf);
              });
            }
          }, 'image/jpeg', 0.5);
          
          // Send heartbeat status
          socket.emit('stream-status');
        }
      }
    }, 100); // 10fps
  };

  const stopVideoEncodeLoop = () => {
    if (videoIntervalRef.current) {
      clearInterval(videoIntervalRef.current);
      videoIntervalRef.current = null;
    }
    if (videoStreamRef.current) {
      videoStreamRef.current.getTracks().forEach(t => t.stop());
      videoStreamRef.current = null;
    }
    if (videoElRef.current) {
      videoElRef.current.srcObject = null;
    }
  };

  const toggleWebcam = async () => {
    initAudio();
    if (isWebcamActive) {
      stopVideoEncodeLoop();
      setIsWebcamActive(false);
    } else {
      try {
        if (isScreenActive) {
          stopVideoEncodeLoop();
          setIsScreenActive(false);
        }
        const stream = await navigator.mediaDevices.getUserMedia({ video: { width: 640, height: 480 } });
        videoStreamRef.current = stream;
        startVideoEncodeLoop(stream);
        setIsWebcamActive(true);
      } catch (err) {
        console.error('Webcam error:', err);
        alert(t.noCameraAccess);
      }
    }
  };

  const toggleScreen = async () => {
    initAudio();
    if (isScreenActive) {
      stopVideoEncodeLoop();
      setIsScreenActive(false);
    } else {
      try {
        if (isWebcamActive) {
          stopVideoEncodeLoop();
          setIsWebcamActive(false);
        }
        const stream = await navigator.mediaDevices.getDisplayMedia({ video: true });
        videoStreamRef.current = stream;
        
        // Handle native "stop sharing" button in browser
        stream.getVideoTracks()[0].onended = () => {
          stopVideoEncodeLoop();
          setIsScreenActive(false);
        };
        
        startVideoEncodeLoop(stream);
        setIsScreenActive(true);
      } catch (err) {
        console.error('Screen error:', err);
        alert(t.noScreenAccess);
      }
    }
  };

  const handleVideoDragStart = (e: React.PointerEvent, userId: string) => {
    e.preventDefault();
    const startX = e.clientX;
    const startY = e.clientY;
    const initialPos = videoPositions[userId] || { x: 0, y: 0 };

    const handlePointerMove = (moveEvent: PointerEvent) => {
      moveEvent.preventDefault();
      const dx = moveEvent.clientX - startX;
      const dy = moveEvent.clientY - startY;
      setVideoPositions(prev => ({
        ...prev,
        [userId]: {
          x: initialPos.x + dx,
          y: initialPos.y + dy
        }
      }));
    };

    const handlePointerUp = () => {
      window.removeEventListener('pointermove', handlePointerMove);
      window.removeEventListener('pointerup', handlePointerUp);
    };

    window.addEventListener('pointermove', handlePointerMove);
    window.addEventListener('pointerup', handlePointerUp);
  };

  return (
    <div className={`window ${featureFlags.showFrame ? '' : 'no-frame'}`}>
      {/* Title Bar */}
      {featureFlags.showFrame && (
        <div className="title-bar">
          <div className="title-bar-text">
            <img src="/favicon.ico" 
                 style={{width: '14px', height: '14px', verticalAlign: 'middle', marginRight: '4px', imageRendering: 'pixelated'}} />
            chat.exe - [{roomId}]{isPrivateRoom ? ' (非公開)' : ''}
          </div>
          <div className="title-bar-controls">
            <button className="title-btn min-btn" aria-label="Minimize"></button>
            <button className="title-btn max-btn" aria-label="Maximize"></button>
            <button className="title-btn close-btn" aria-label="Close"></button>
          </div>
        </div>
      )}
      
      {/* Window Body */}
      <div className="window-body">
        <div className="main-area">
          
          {/* Left Area: Chat Log + Input */}
          <div className="left-area">
            {/* Chat Log */}
            <div className="chat-log sunken-panel" ref={chatLogRef}>
              {messages.map(msg => {
                const cleanName = msg.userName.split('#')[0];
                const msgStyle = {
                  fontWeight: msg.b ? 'bold' : 'normal',
                  fontStyle: msg.i ? 'italic' : 'normal',
                  textDecoration: msg.u ? 'underline' : 'none',
                  fontSize: msg.sz ? `${msg.sz}pt` : '10pt', // Win32 sizes are typically in points
                  color: getColor(msg.clr)
                };
                return (
                  <div key={msg.id} style={{ marginBottom: '2px' }}>
                    {msg.isSystem ? (
                      <span style={{ color: msg.systemColor, fontSize: '9pt' }}>
                        {msg.msg}
                      </span>
                    ) : (
                      <>
                        <span style={{ color: getColor(msg.nclr !== undefined ? msg.nclr : msg.clr), fontWeight: 'bold' }}>
                          {cleanName}:
                        </span>{' '}
                        <span style={msgStyle}>
                          {renderMessageWithLinks(msg.msg)}
                        </span>
                      </>
                    )}
                  </div>
                );
              })}
            </div>
            
            {/* Toolbar */}
            <div className="toolbar">
              <button type="button" className={`tool-btn ${isBold ? 'active' : ''}`} onClick={() => setIsBold(!isBold)}>B</button>
              <button type="button" className={`tool-btn ${isItalic ? 'active' : ''}`} style={{ fontStyle: 'italic' }} onClick={() => setIsItalic(!isItalic)}>I</button>
              <button type="button" className={`tool-btn ${isUnderline ? 'active' : ''}`} style={{ textDecoration: 'underline' }} onClick={() => setIsUnderline(!isUnderline)}>U</button>
              <select className="tool-select" value={fontSize} onChange={(e) => setFontSize(parseInt(e.target.value, 10))}>
                {[6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 32].map(size => (
                  <option key={size} value={size}>{size}</option>
                ))}
              </select>
              <button 
                type="button"
                className="tool-btn"
                style={{ padding: '2px 4px', margin: '0 4px', cursor: 'pointer' }}
                onClick={(e) => {
                  const rect = e.currentTarget.getBoundingClientRect();
                  setColorPickerPos({ x: rect.left, y: window.innerHeight - rect.top + 4 });
                  setTempMsgColor(msgColor);
                  setColorPickerTarget('msg');
                  setShowColorPicker(true);
                }}
              >
                <div className="color-box" style={{ backgroundColor: getHexColor(msgColor), margin: 0, display: 'inline-block' }}></div>
              </button>
              {featureFlags.enableMic && (
                <>
                  <button 
                    type="button" 
                    className={`tool-btn mic-btn ${isMicActive ? 'active' : ''}`} 
                    onClick={toggleMic}
                    style={{ backgroundColor: isMicActive ? '#64ff64' : '#c0c0c0', display: 'flex', alignItems: 'center', gap: '4px' }}
                  >
                    <MicIcon /> {t.mic}
                  </button>
                  {isMicActive && (
                    <div className="sunken-panel" style={{ width: '70px', height: '22px', marginLeft: '4px', position: 'relative', overflow: 'hidden', backgroundColor: '#c0c0c0' }}>
                      <div style={{
                        position: 'absolute',
                        left: 0,
                        top: 0,
                        height: '100%',
                        width: '100%',
                        background: 'linear-gradient(to right, #00dc00 0%, #ffdc00 70%, #ff0000 100%)',
                        clipPath: `inset(0 ${100 - micLevel}% 0 0)`,
                        transition: 'clip-path 0.1s linear'
                      }}></div>
                    </div>
                  )}
                </>
              )}
              {featureFlags.enableAudio && (
                <div style={{ marginLeft: '12px', display: 'flex', alignItems: 'center' }}>
                  <button 
                    className={`speaker-btn ${volume === 0 ? 'muted' : ''}`} 
                    title={volume === 0 ? "Unmute" : "Mute"} 
                    onClick={() => setVolume(volume === 0 ? 50 : 0)} 
                    aria-label="Toggle Mute"
                  ></button>
                  <input 
                    type="range" 
                    min="0" 
                    max="100" 
                    value={volume} 
                    onChange={(e) => setVolume(Number(e.target.value))}
                    style={{ width: '80px' }} 
                  />
                </div>
              )}
            </div>

            {/* Input Area */}
            <form className="input-area" onSubmit={sendMessage}>
              <textarea 
                ref={chatInputRef}
                autoFocus
                className="chat-input sunken-panel" 
                value={messageInput}
                onChange={e => setMessageInput(e.target.value)}
                onKeyDown={e => {
                  if (e.key === 'Enter' && !e.shiftKey) {
                    if (e.nativeEvent.isComposing) return;
                    e.preventDefault();
                    sendMessage(e as any);
                  }
                }}
                style={{
                  fontWeight: isBold ? 'bold' : 'normal',
                  fontStyle: isItalic ? 'italic' : 'normal',
                  textDecoration: isUnderline ? 'underline' : 'none',
                  fontSize: `${fontSize}pt`,
                  color: getColor(msgColor)
                }}
              />
              <button type="submit" className="send-btn" disabled={!isJoined || !messageInput.trim()}>{t.send}</button>
            </form>
          </div>

          {/* Right Area: Users + Move Room */}
          {featureFlags.showUsers && (
            <div className="right-area">
              <div className="user-list-container sunken-panel">
                <div className="user-list-header">
                  {t.users} ({users.length})
                </div>
                <div className="user-list">
                  {[...users].sort((a, b) => {
                    const aIgnored = ignoredUsers.includes(a.userId);
                    const bIgnored = ignoredUsers.includes(b.userId);
                    if (aIgnored && !bIgnored) return 1;
                    if (!aIgnored && bIgnored) return -1;
                    return a.userName.localeCompare(b.userName);
                  }).map(u => {
                    const isSelf = u.userId === localUserId;
                    const cleanName = u.userName.split('#')[0];
                    const nclr = u.nclr !== undefined ? u.nclr : 0;
                    const isIgnored = ignoredUsers.includes(u.userId);
                    return (
                      <div 
                        key={u.userId} 
                        className={`user-item ${selectedUserId === u.userId ? 'selected' : ''}`}
                        onContextMenu={(e) => {
                          e.preventDefault();
                          e.stopPropagation();
                          setSelectedUserId(u.userId);
                          const x = Math.min(e.clientX, window.innerWidth - 180);
                          const y = Math.min(e.clientY, window.innerHeight - 150);
                          setContextMenu({ x, y, userId: u.userId, isSelf });
                        }}
                        onClick={(e) => {
                          e.preventDefault();
                          e.stopPropagation();
                          setSelectedUserId(u.userId);
                          const x = Math.min(e.clientX, window.innerWidth - 180);
                          const y = Math.min(e.clientY, window.innerHeight - 150);
                          setContextMenu({ x, y, userId: u.userId, isSelf });
                        }}
                      >
                        <div style={{ display: 'flex', alignItems: 'center' }}>
                          {featureFlags.enableVideo && (streamingUsers[u.userId] || (isSelf && (isWebcamActive || isScreenActive))) ? (
                            <div className="stream-icon-container" style={{ marginRight: '4px', filter: isIgnored ? 'grayscale(100%) opacity(70%)' : 'none' }}>
                              <span className="stream-icon-tv">📺</span>
                              <span className="stream-icon-face">🙂</span>
                            </div>
                          ) : (
                            <span className="user-icon" style={{ marginRight: '4px', filter: isIgnored ? 'grayscale(100%) opacity(70%)' : 'none' }}>🙂</span>
                          )}
                          <span style={{ color: selectedUserId === u.userId ? '#ffffff' : getColor(isSelf ? nclr : u.nclr), fontWeight: isSelf ? 'bold' : 'normal' }}>
                            {isSelf ? name.split('#')[0] : cleanName} {isSelf ? `(${t.self})` : ''}
                          </span>
                          {talkingUsers[u.userId] && <span style={{ marginLeft: '4px', display: 'flex', alignItems: 'center' }}><MicIcon /></span>}
                        </div>
                      </div>
                    );
                  })}
                </div>
              </div>
              {featureFlags.showMoveRoom && (
                <button className="tool-btn" style={{ width: '100%', marginTop: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', gap: '6px' }} onClick={_openMoveRoom}>
                  <svg width="16" height="16" viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
                    <rect x="2" y="2" width="8" height="12" />
                    <circle cx="7" cy="8" r="1" fill="currentColor" stroke="none" />
                    <line x1="10" y1="8" x2="16" y2="8" />
                    <line x1="16" y1="8" x2="13" y2="5" />
                    <line x1="16" y1="8" x2="13" y2="11" />
                  </svg>
                  {t.moveRoomBtn}
                </button>
              )}
            </div>
          )}
          
        </div>
      </div>

      {/* Context Menu */}
      {contextMenu && (
        <div 
          className="context-menu" 
          style={{ position: 'fixed', left: contextMenu.x, top: contextMenu.y, zIndex: 1000 }}
          onContextMenu={(e) => e.preventDefault()}
        >
          {contextMenu.isSelf ? (
            <>
              {featureFlags.enableCamera && (
                <div className="context-menu-item" onClick={() => { toggleWebcam(); setContextMenu(null); }}>
                  {isWebcamActive ? t.stopCamera : t.startCamera}
                </div>
              )}
              {featureFlags.enableScreen && (
                <div className="context-menu-item" onClick={() => { toggleScreen(); setContextMenu(null); }}>
                  {t.shareScreen}
                </div>
              )}
              <div className="context-menu-separator"></div>
              <div className="context-menu-item" onClick={() => {
                setShowChangeName(true);
                setNewNameInput(name);
                setNewColorInput(nclr);
                setContextMenu(null);
              }}>{t.changeName}</div>
            </>
          ) : (
            <>
              {featureFlags.enableVideo && (
                <div 
                  className={`context-menu-item ${!streamingUsers[contextMenu.userId] ? 'disabled' : ''}`} 
                  onClick={() => {
                    if (streamingUsers[contextMenu.userId]) {
                      setWatchingUsers(prev => ({ ...prev, [contextMenu.userId]: true }));
                      socket?.emit('watch-video', contextMenu.userId);
                      setContextMenu(null);
                    }
                  }}
                >
                  {t.watchVideo}
                </div>
              )}
              <div className="context-menu-separator"></div>
              <div className="context-menu-item" onClick={() => { 
                setIgnoredUsers(prev => {
                  if (prev.includes(contextMenu.userId)) {
                    return prev.filter(id => id !== contextMenu.userId);
                  } else {
                    setWatchingUsers(w => {
                      const nw = { ...w };
                      delete nw[contextMenu.userId];
                      return nw;
                    });
                    return [...prev, contextMenu.userId];
                  }
                });
                setContextMenu(null); 
              }}>
                {ignoredUsers.includes(contextMenu.userId) ? '✓ ' : ''}{t.ignore}
              </div>
              <div className="context-menu-item" onClick={() => { 
                setIgnoredUsers(prev => {
                  if (!prev.includes(contextMenu.userId)) {
                    setWatchingUsers(w => {
                      const nw = { ...w };
                      delete nw[contextMenu.userId];
                      return nw;
                    });
                    return [...prev, contextMenu.userId];
                  }
                  return prev;
                });
                socket?.emit('report', { targetId: contextMenu.userId });
                alert(t.reported); 
                setContextMenu(null); 
              }}>{t.report}</div>
            </>
          )}
        </div>
      )}

      {/* Move Room Dialog */}
      {showMoveRoom && (
        <div className="dialog-overlay">
          <div className="dialog window">
            <div className="title-bar">
              <div className="title-bar-text">{t.moveRoom}</div>
              <div className="title-bar-controls">
                <button className="title-btn close-btn" onClick={closeMoveRoom}></button>
              </div>
            </div>
            <div className="window-body">
              <form onSubmit={handleMoveRoomSubmit}>
                <div style={{ marginBottom: '8px' }}>
                  <label>{t.roomName}:</label>
                  <input 
                    type="text" 
                    className="chat-input sunken-panel" 
                    style={{ width: '100%', marginTop: '4px' }}
                    value={newRoomName}
                    onChange={(e) => setNewRoomName(e.target.value)}
                    autoFocus
                  />
                  <div style={{ marginTop: '4px' }}>
                    <label>
                      <input 
                        type="checkbox" 
                        checked={isPrivateRoom} 
                        onChange={(e) => setIsPrivateRoom(e.target.checked)} 
                      /> {t.privateRoom}
                    </label>
                  </div>
                </div>
                
                <div style={{ marginBottom: '4px' }}>{t.publicRooms}:</div>
                <select 
                  size={5} 
                  style={{ width: '100%', marginBottom: '12px', padding: '2px', backgroundColor: '#fff' }}
                  className="sunken-panel"
                  onChange={(e) => setNewRoomName(e.target.value)}
                >
                  {[...publicRooms].sort((a, b) => b.count - a.count || a.name.localeCompare(b.name)).map(r => (
                    <option key={r.name} value={r.name}>{r.name} ({r.count}{t.people})</option>
                  ))}
                </select>

                <div className="dialog-buttons">
                  <button type="submit" className="sys-btn" style={{ minWidth: '60px' }}>{t.move.replace(/\(&.\)/g, '')}</button>
                  <button type="button" className="sys-btn" style={{ minWidth: '80px' }} onClick={closeMoveRoom}>{t.cancel}</button>
                </div>
              </form>
            </div>
          </div>
        </div>
      )}

      {/* Change Name Dialog */}
      {showChangeName && (
        <div className="dialog-overlay">
          <div className="dialog window">
            <div className="title-bar">
              <div className="title-bar-text">{t.changeName}</div>
              <div className="title-bar-controls">
                <button className="title-btn close-btn" onClick={() => setShowChangeName(false)}></button>
              </div>
            </div>
            <div className="window-body">
              <form onSubmit={handleChangeNameSubmit}>
                <div style={{ marginBottom: '8px' }}>
                  <label>{t.name}:</label>
                  <input 
                    type="text" 
                    className="chat-input sunken-panel" 
                    style={{ width: '100%', marginTop: '4px' }}
                    value={newNameInput}
                    onChange={(e) => setNewNameInput(e.target.value)}
                    autoFocus
                  />
                </div>
                <div style={{ marginBottom: '12px' }}>
                  <label>{t.color}:</label>
                  <div style={{ display: 'flex', gap: '4px', marginTop: '4px', flexWrap: 'wrap' }}>
                    {extendedColors.map((hex, i) => {
                      const colorRef = toColorRefFromHex(hex);
                      return (
                        <div 
                          key={i} 
                          onClick={() => setNewColorInput(colorRef)}
                          style={{
                            width: '16px', height: '16px', backgroundColor: hex, 
                            border: newColorInput === colorRef ? '2px solid #000' : '1px solid #808080',
                            cursor: 'pointer'
                          }} 
                        />
                      );
                    })}
                  </div>
                  <div style={{ display: 'flex', alignItems: 'center', gap: '4px', marginTop: '8px' }}>
                    <span style={{ fontSize: '11px' }}>{t.custom}:</span>
                    <input 
                      type="color" 
                      value={getHexColor(newColorInput)}
                      onChange={(e) => setNewColorInput(toColorRefFromHex(e.target.value))}
                      style={{ height: '24px', padding: 0, flexGrow: 1 }}
                    />
                  </div>
                </div>
                <div className="dialog-buttons">
                  <button type="submit" className="sys-btn" style={{ minWidth: '60px' }}>{t.change}</button>
                  <button type="button" className="sys-btn" style={{ minWidth: '80px' }} onClick={() => setShowChangeName(false)}>{t.cancel}</button>
                </div>
              </form>
            </div>
          </div>
        </div>
      )}

      {/* Video Windows */}
      {Object.entries(watchingUsers).filter(([_, isWatching]) => isWatching).map(([userId]) => {
        const u = users.find(u => u.userId === userId);
        const cleanName = u ? u.userName.split('#')[0] : 'User';
        const hasFrame = !!videoFrames[userId];
        const pos = videoPositions[userId] || { x: 0, y: 0 };
        return (
          <div key={userId} className="window video-window" style={{ position: 'fixed', top: '50px', right: '20px', zIndex: 1000, boxShadow: '2px 2px 5px rgba(0,0,0,0.5)', transform: `translate(${pos.x}px, ${pos.y}px)` }}>
            <div className="title-bar" onPointerDown={(e) => handleVideoDragStart(e, userId)} style={{ cursor: 'move' }}>
              <div className="title-bar-text">{cleanName} - {t.videoStream} ({viewerCounts[userId] || 0})</div>
              <div className="title-bar-controls">
                <button className="title-btn close-btn" aria-label="Close" onClick={() => {
                  setWatchingUsers(prev => ({ ...prev, [userId]: false }));
                  const jmuxer = jmuxersRef.current.get(userId);
                  if (jmuxer) {
                    try { jmuxer.destroy(); } catch (e) {}
                    jmuxersRef.current.delete(userId);
                  }
                  setVideoFrames(prev => {
                    const newFrames = { ...prev };
                    delete newFrames[userId];
                    return newFrames;
                  });
                }}></button>
              </div>
            </div>
            <div className="window-body" style={{ margin: 0, padding: 0, background: '#000', height: '180px', display: 'flex', justifyContent: 'center', alignItems: 'center', position: 'relative' }}>
            
              <video 
                id={`video-element-${userId}`} 
                autoPlay 
                muted 
                playsInline
                style={{ width: '100%', height: '100%', objectFit: 'contain', display: hasFrame && videoFrames[userId] === 'h264' ? 'block' : 'none' }} 
              />
              {hasFrame && videoFrames[userId] !== 'h264' && videoFrames[userId].startsWith('blob:') && (
                <img src={videoFrames[userId]} alt="video" style={{ width: '100%', height: '100%', objectFit: 'contain', display: 'block' }} />
              )}
              {!hasFrame && (
                <div style={{ color: '#fff', fontSize: '12px' }}>{t.loading}...</div>
              )}
            </div>
          </div>
        );
      })}

      {/* Self Preview Video Window */}
      {showSelfPreview && (
        <div className="window video-window" style={{ position: 'fixed', top: '50px', left: '20px', zIndex: 1000, boxShadow: '2px 2px 5px rgba(0,0,0,0.5)', transform: `translate(${videoPositions['self']?.x || 0}px, ${videoPositions['self']?.y || 0}px)` }}>
          <div className="title-bar" onPointerDown={(e) => handleVideoDragStart(e, 'self')} style={{ cursor: 'move' }}>
            <div className="title-bar-text">{t.self} - {t.videoStream} {t.viewingCount.replace('%d', (viewerCounts[localUserIdRef.current] || 0).toString())}</div>
            <div className="title-bar-controls">
              <button className="title-btn close-btn" aria-label="Close" onClick={() => {
                setShowSelfPreview(false);
                stopVideoEncodeLoop();
                setIsWebcamActive(false);
                setIsScreenActive(false);
              }}></button>
            </div>
          </div>
          <div className="window-body" style={{ margin: 0, padding: 0, background: '#000', height: '180px', display: 'flex', justifyContent: 'center', alignItems: 'center', position: 'relative' }}>
            <video 
              ref={selfVideoRef}
              autoPlay 
              muted 
              playsInline
              style={{ width: '100%', height: '100%', objectFit: 'contain' }} 
            />
          </div>
        </div>
      )}

      {/* Color Picker Dialog */}
      {showColorPicker && (
        <div 
          className="window"
          style={{
            position: 'fixed',
            left: colorPickerPos.x,
            bottom: colorPickerPos.y,
            zIndex: 2000,
            padding: '4px',
            width: '180px',
            height: 'auto',
            boxShadow: '2px 2px 5px rgba(0,0,0,0.5)'
          }}
        >
          <div className="title-bar" style={{ padding: '2px', marginBottom: '4px' }}>
            <div className="title-bar-text">{t.selectColor}</div>
            <div className="title-bar-controls">
              <button className="title-btn close-btn" aria-label="Close" onClick={() => setShowColorPicker(false)}></button>
            </div>
          </div>
          <div className="window-body" style={{ margin: 0, padding: 0, height: 'auto', display: 'flex', flexDirection: 'column', gap: '8px' }}>
            <div style={{ display: 'flex', flexWrap: 'wrap', gap: '4px' }}>
              {extendedColors.map((hex, i) => {
                const colorRef = toColorRefFromHex(hex);
                return (
                  <div 
                    key={i} 
                    onClick={() => setTempMsgColor(colorRef)}
                    style={{ 
                      width: '16px', height: '16px', 
                      backgroundColor: hex, 
                      border: tempMsgColor === colorRef ? '2px solid #000' : '1px solid #808080',
                      boxShadow: tempMsgColor === colorRef ? 'inset 1px 1px #fff, inset -1px -1px #fff' : 'none',
                      cursor: 'pointer' 
                    }}
                  />
                );
              })}
            </div>
            <div style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
              <span style={{ fontSize: '11px' }}>{t.custom}:</span>
              <input 
                type="color" 
                value={getHexColor(tempMsgColor)}
                onChange={(e) => setTempMsgColor(toColorRefFromHex(e.target.value))}
                style={{ height: '24px', padding: 0, flexGrow: 1 }}
              />
            </div>
            <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '4px', marginTop: '4px' }}>
              <button 
                type="button" 
                onClick={() => { 
                  if (colorPickerTarget === 'msg') setMsgColor(tempMsgColor); 
                  if (colorPickerTarget === 'name') setNewColorInput(tempMsgColor);
                  setShowColorPicker(false); 
                }}
                style={{ width: '50px' }}
              >OK</button>
              <button 
                type="button" 
                onClick={() => setShowColorPicker(false)}
                style={{ width: 'auto', padding: '0 8px', whiteSpace: 'nowrap' }}
              >キャンセル</button>
            </div>
          </div>
        </div>
      )}

    </div>
  );
};

export default App;
