import React, { useEffect } from 'react';
import { useState } from "react"
import { useAppDispatch, useAppSelector } from "../../app/hooks"
import { chunks, addChunk} from './gameWindow';
import Container from 'react-bootstrap/Container';
import Button from 'react-bootstrap/Button';
import Form from 'react-bootstrap/Form';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import io from 'https://cdn.socket.io/4.7.4/socket.io.esm.min.js';

export const GameWindow = () => {
    const dispatch = useAppDispatch();
    const textChunks = useAppSelector(chunks);
    const protocol = window.location.protocol.includes('https') ? 'https' : 'http';
    const host = window.location.hostname;
    //const port = window.location.port || (protocol === 'https' ? 443 : 80);
    const port = 8000;
    const socketUrl = `${protocol}://${host}:${port}`;
    const socket = io(socketUrl, { autoConnect: false });

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
            socket.disconnect();
        };
    }, []);

    // Create a single HTML string from all events
    const createMarkup = () => {
        return { __html: textChunks.join('') };
    };

    return (
        <Container fluid id="mainapp">
            <Row id="gametext" dangerouslySetInnerHTML={createMarkup()} />
            <Row id="gamecontrols">
                <Form>
                    <Row className="align-items-center">
                        <Col xs={9} md={10}> {/* Adjust the size props as necessary for responsiveness */}
                            <Form.Group controlId="gameSubmit">
                                <Form.Control type="text" placeholder="Enter command" />
                            </Form.Group>
                        </Col>
                        <Col xs={2} md={2}> {/* Adjust the size props as necessary for responsiveness */}
                            <Button variant="primary">
                                Send
                            </Button>
                        </Col>
                    </Row>
                </Form>
            </Row>
        </Container>

    );
};
