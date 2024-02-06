import React, { useRef, useEffect } from 'react';
import { chunks } from './gameWindow';
import {  useAppSelector } from "../../app/hooks";

export const GameText = () => {
    const textChunks = useAppSelector(chunks);
    const gameTextEndRef = useRef(null);

    useEffect(() => {
        if (gameTextEndRef.current) {
            (gameTextEndRef.current as HTMLDivElement).scrollIntoView({ behavior: 'smooth' });
        }
    }, [textChunks]);

    // Create a single HTML string from all events
    const createMarkup = () => {
        return { __html: textChunks.join('') };
    };

    return (
            <div className="gametextholder">
                <div className="gametext" dangerouslySetInnerHTML={createMarkup()} />
                <div className="bottomref" ref={gameTextEndRef} />
            </div>
    );
}