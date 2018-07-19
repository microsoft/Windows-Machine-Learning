import { createStore } from 'redux'

import { rootReducer } from './reducers';

const browserGlobal = window as any;

const store = createStore(rootReducer, browserGlobal.__REDUX_DEVTOOLS_EXTENSION__ && browserGlobal.__REDUX_DEVTOOLS_EXTENSION__());
export default store;
