import * as React from 'react';
import * as ReactDOM from 'react-dom';
import { Provider } from 'react-redux'

import App from './App';
import store from './datastore/store';
import './index.css';
import { registerKeyboardShurtcuts } from './native/menu';
import registerServiceWorker from './registerServiceWorker';

ReactDOM.render(
    <Provider store={store}>
        <App />
    </Provider>,
    document.getElementById('root') as HTMLElement
);
registerKeyboardShurtcuts();
registerServiceWorker();
