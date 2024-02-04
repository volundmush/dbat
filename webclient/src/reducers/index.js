import { combineReducers } from 'redux';
import GameTextReducer from './GameTextReducer.js';
// other reducers you might have

export const rootReducer = combineReducers({
  gametext: GameTextReducer,
  // other reducers
});