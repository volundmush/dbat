import React, { useState, ChangeEventHandler, KeyboardEventHandler } from 'react';

import {socket} from "./SocketIO";

export const GameInput = () => {
    const [command, setCommand] = useState('');

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

    

    return (
        <textarea className="gameinput"
            autoComplete="off"
            placeholder="Enter command"
            value={command}
            onChange={handleInputChange}
            onKeyDown={handleKeyDown}
            autoFocus={true}
        />
    );

};