import { IAction, SET_INPUTS, SET_METADATA_PROPS } from '../actions';
import { ModelProtoSingleton } from './modelProto';

export const protoMiddleware = (store: any) => (next: (action: IAction) => any) => (action: IAction) => {
    switch (action.type) {
        case SET_INPUTS:
            ModelProtoSingleton.setInputs(action.inputs);
            return next(action);
        case SET_METADATA_PROPS:
            ModelProtoSingleton.setMetadata(action.metadataProps);
            return next(action);
    }
    return next(action);
}
