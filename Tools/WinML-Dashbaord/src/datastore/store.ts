import { applyMiddleware, createStore } from 'redux'

import { protoMiddleware } from './proto/middleware';
import { rootReducer } from './reducers';

const browserGlobal = window as any;

const store = createStore(rootReducer, browserGlobal.__REDUX_DEVTOOLS_EXTENSION__ && browserGlobal.__REDUX_DEVTOOLS_EXTENSION__(), applyMiddleware(protoMiddleware));
export default store;
