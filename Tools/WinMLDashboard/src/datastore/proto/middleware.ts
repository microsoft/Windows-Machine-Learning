import { IAction, SET_DEBUG_NODES, SET_INPUTS, SET_METADATA_PROPS, SET_OUTPUTS } from '../actions';
import { ModelProtoSingleton } from './modelProto';

export const protoMiddleware = (store: any) => (next: (action: IAction) => any) => (action: IAction) => {
    switch (action.type) {
        case SET_DEBUG_NODES:
            ModelProtoSingleton.setDebugNodes(action.debugNodes);
            break;
        case SET_INPUTS:
            ModelProtoSingleton.setInputs(action.inputs);
            break;
        case SET_METADATA_PROPS:
            ModelProtoSingleton.setMetadata(action.metadataProps);
            break;
        case SET_OUTPUTS:
            ModelProtoSingleton.setOutputs(action.outputs);
            break;
    }
    return next(action);
}
