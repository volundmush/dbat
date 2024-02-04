import Stack from 'react-bootstrap/Stack';
import Button from 'react-bootstrap/Button';
import Form from 'react-bootstrap/Form';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';

export function GameInput() {
  return (
    <Form>
      <Row className="align-items-center">
        <Col xs={9} md={10}> {/* Adjust the size props as necessary for responsiveness */}
          <Form.Group controlId="formBasicEmail">
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
  );
}