import React, { useEffect, useState, useRef, ChangeEventHandler, KeyboardEventHandler} from 'react';
import { useAppDispatch, useAppSelector } from "../../app/hooks"
import { chunks, addChunk} from './gameWindow';
import Container from 'react-bootstrap/Container';
import Button from 'react-bootstrap/Button';
import Form from 'react-bootstrap/Form';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import io from 'socket.io-client';
import Stack from 'react-bootstrap/Stack';
import { SplitView } from '@swc-react/split-view';

const protocol = window.location.protocol.includes('https') ? 'https' : 'http';
const host = window.location.hostname;
const port = (import.meta.env.MODE === "production") ? (window.location.port || (protocol === 'https' ? 443 : 80)) : 8000;
const socketUrl = `${protocol}://${host}:${port}`;
const socket = io(socketUrl, { autoConnect: false });

export const GameWindow = () => {
    const dispatch = useAppDispatch();
    const [command, setCommand] = useState('');
    const textChunks = useAppSelector(chunks);
    const gameTextEndRef = useRef(null);
    
    useEffect(() => {
        console.log(`Attempting to connect SocketIO to ${socketUrl}`);

        socket.on('connect', () => {
            console.log('Connected to Socket.IO server at', socketUrl);
        });

        socket.on('disconnect', () => {
            console.log('Remote Server disconnected.');
        });

        socket.onAny((eventName: string, message: any) => {
            console.log(`Received event: ${eventName}`, message);
            if(eventName === "Game.Text") {
                dispatch(addChunk(message.data));
            }

        });
        socket.connect();

        return () => {
            console.log('Disconnecting from Socket.IO server.');
            //socket.disconnect();
        };
    }, []);

    const handleInputChange: ChangeEventHandler<HTMLTextAreaElement> = (e) => {
        setCommand(e.target.value);
    };
    
    const handleKeyDown: KeyboardEventHandler<HTMLTextAreaElement> = (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {  // Prevents submission if Shift+Enter is pressed
            e.preventDefault();  // Prevents the default action of Enter key in a textarea (new line)
            handleSubmit(e);
        }
    };

    const handleSubmit = (e: React.FormEvent<HTMLFormElement> | React.KeyboardEvent<HTMLTextAreaElement>) => {
        e.preventDefault();
        if (command.trim()) {
            socket.emit("Game.Command", { data: command.trim() });
            setCommand(''); // Clear the input after sending the command
        }
    };

    useEffect(() => {
        if (gameTextEndRef.current) {
            (gameTextEndRef.current as HTMLDivElement).scrollIntoView({ behavior: 'smooth'});
        }
    }, [textChunks]);

    // Create a single HTML string from all events
    const createMarkup = () => {
        return { __html: textChunks.join('') };
    };

    return (
            <SplitView id="gamedisplay"
             resizable
             primarySize="40%"
             >
            <SplitView id="gametextdisplay"
            resizable 
            vertical
            primarySize="80%"
            >
                <div id="gametextholder">
                    <div id="gametext" dangerouslySetInnerHTML={createMarkup()}/>
                    <div id="bottomref" ref={gameTextEndRef}/>
                </div>
            <textarea id="gameinput" 
                autoComplete="off" 
                placeholder="Enter command"
                value={command}
                onChange={handleInputChange}
                onKeyDown={handleKeyDown}
                autoFocus={true}
                />
        </SplitView>

            <div id="gamedatadisplay">
            Blargh.
            </div>
        </SplitView>
      );
};
