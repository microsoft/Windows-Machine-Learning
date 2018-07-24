import { IAction, UPDATE_METADATA_PROPS } from '../actions';
import { ModelProtoSingleton } from './modelProto';

export const protoMiddleware = (store: any) => (next: (action: IAction) => any) => (action: IAction) => {
    if (action.type === UPDATE_METADATA_PROPS) {
        ModelProtoSingleton.setMetadata(action.metadataProps);
    }
    return next(action);
}
