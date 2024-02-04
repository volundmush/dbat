import './App.css';
import {GameWindow} from './components/GameWindow.js';
import {GameInput} from './components/GameInput.js';
import {GameNav} from './components/GameNav.js';
import Container from 'react-bootstrap/Container';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import Stack from 'react-bootstrap/Stack';



function App() {

  return (
    <Container fluid id="mainapp">
      <Row>
        <GameNav />
      </Row>
      <Row id="gamewindowholder">
      <GameWindow />
      </Row>
      <Row>
        <GameInput />
      </Row>
    </Container>
  );
}

export default App;
