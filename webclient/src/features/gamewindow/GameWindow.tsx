import React, { useEffect } from 'react';
import { useAppDispatch } from "../../app/hooks";
import { addChunk } from './gameWindow';
import { Mosaic, MosaicWindow, MosaicNode } from 'react-mosaic-component';
import {socket} from "./SocketIO";
import {GameText} from "./GameText";
import {GameInput} from "./GameInput";
import { Classes, HTMLSelect } from '@blueprintjs/core';
import classNames from 'classnames';

const THEMES = {
    ['Blueprint']: 'mosaic-blueprint-theme',
    ['Blueprint Dark']: classNames('mosaic-blueprint-theme', Classes.DARK),
    ['None']: '',
  };

type Theme = keyof typeof THEMES;

const ELEMENT_MAP: { [viewId: string]: JSX.Element } = {
    gametext: <GameText/>,
    gameinput: <GameInput/>,
};

export interface ExampleAppState {
    currentNode: MosaicNode<string> | null;
    currentTheme: Theme;
  }

export class GameWindow extends React.PureComponent<{}, > {
    const dispatch = useAppDispatch();
    
    useEffect(() => {
        console.log(`Attempting to connect SocketIO`);

        socket.on("Game.Text", (message) => {
            console.log(`Received Game.Text:`, message);
            if(message.data)
                dispatch(addChunk(message.data));

        });

        socket.connect();

        return () => {
            console.log('Disconnecting from Socket.IO server.');
            socket.disconnect();
        };
    }, []);

    return (
        <Mosaic<string>
            renderTile={(id, path) => (
                <MosaicWindow<string> path={path} createNode={() => 'new'} title={id}>
                    {ELEMENT_MAP[id]}
                </MosaicWindow>
            )}
            initialValue={{
                direction: 'column',
                first: "gametext",
                second: 'gameinput',
                splitPercentage: 80,
            }}
        />
    );
};
