import io from 'socket.io-client';
const protocol = window.location.protocol.includes('https') ? 'https' : 'http';
const host = window.location.hostname;
const port = (import.meta.env.MODE === "production") ? (window.location.port || (protocol === 'https' ? 443 : 80)) : 8000;
const socketUrl = `${protocol}://${host}:${port}`;
export const socket = io(socketUrl, { autoConnect: false });

socket.on('connect', () => {
    console.log('Connected to Socket.IO server at', socketUrl);
});

socket.on('disconnect', () => {
    console.log('Remote Server disconnected.');
});