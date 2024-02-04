import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import App from './App';
import reportWebVitals from './reportWebVitals';
import {store} from './app/store.js';
import { Provider } from 'react-redux'
import 'bootstrap/dist/css/bootstrap.min.css';
import { SocketProvider } from './components/SocketIO.js';

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(

    <Provider store={store}>
      <SocketProvider>
      <App />
      </SocketProvider>
    </Provider>

);

// If you want to start measuring performance in your app, pass a function
// to log results (for example: reportWebVitals(console.log))
// or send to an analytics endpoint. Learn more: https://bit.ly/CRA-vitals
reportWebVitals();
