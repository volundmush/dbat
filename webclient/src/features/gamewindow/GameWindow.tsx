import React, { useEffect, useState, useRef, ChangeEventHandler, KeyboardEventHandler } from 'react';
import { useAppDispatch, useAppSelector } from "../../app/hooks"
import { chunks, addChunk } from './gameWindow';
import io from 'socket.io-client';
import { Mosaic, MosaicWindow } from 'react-mosaic-component';


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
            if (eventName === "Game.Text") {
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
            (gameTextEndRef.current as HTMLDivElement).scrollIntoView({ behavior: 'smooth' });
        }
    }, [textChunks]);

    // Create a single HTML string from all events
    const createMarkup = () => {
        return { __html: textChunks.join('') };
    };

    const ELEMENT_MAP: { [viewId: string]: JSX.Element } = {
        a: <div id="gametextdisplay">
            <div id="gametextholder">
                <div id="gametext" dangerouslySetInnerHTML={createMarkup()} />
                <div id="bottomref" ref={gameTextEndRef} />
            </div>

        </div>,
        b: <textarea id="gameinput"
            autoComplete="off"
            placeholder="Enter command"
            value={command}
            onChange={handleInputChange}
            onKeyDown={handleKeyDown}
            autoFocus={true}
        />,
        c: <div>Blargh</div>,
        new: <span>New window</span>,
    };

    return (
        <Mosaic<string>
            renderTile={(id, path) => (
                <MosaicWindow<string> path={path} createNode={() => 'new'} title="booya">
                    {ELEMENT_MAP[id]}
                </MosaicWindow>
            )}
            initialValue={{
                direction: 'row',
                first: {
                    direction: 'column',
                    first: 'a',
                    second: 'b',
                    splitPercentage: 80
                },
                second: 'c',
                splitPercentage: 50,
            }}
        />
    );
};
