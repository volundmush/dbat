import React, { useEffect } from 'react';
import { useDispatch } from 'react-redux';
import {SocketIOContext} from '../contexts/SocketIO.js';
import io from 'https://cdn.socket.io/4.7.4/socket.io.esm.min.js';

// Redux action creator for incoming events
const incomingEvent = (eventName, message) => ({
    type: eventName,
    payload: message,
  });


export const SocketProvider = ({ children }) => {
    const dispatch = useDispatch();
    const protocol = window.location.protocol.includes('https') ? 'https' : 'http';
    const host = window.location.hostname;
    //const port = window.location.port || (protocol === 'https' ? 443 : 80);
    const port = 8000;
    const socketUrl = `${protocol}://${host}:${port}`;
    const socket = io(socketUrl, {autoConnect: false});

  useEffect(() => {
    console.log(`Attempting to connect SocketIO to ${socketUrl}`);

    socket.on('connect', () => {
        console.log('Connected to Socket.IO server at', socketUrl);
      });

      socket.on('disconnect', () => {
        console.log('Remote Server disconnected.');
      });

      socket.onAny((eventName, message) => {
        console.log(`Received event: ${eventName}`, message);
        dispatch(incomingEvent(eventName, message));
    });
    socket.connect();
    
    return () => {
      socket.disconnect();
    };
  }, [dispatch, socket, socketUrl]);

  return (
    <SocketIOContext.Provider value={socket}>
      {children}
    </SocketIOContext.Provider>
  );
};