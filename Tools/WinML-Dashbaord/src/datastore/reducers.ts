import * as actions from './actions';
import IState from './state';

export function rootReducer(state: IState, action: actions.IAction) {
    state = state || {};
    switch (action.type) {
        case actions.SET_FILE:
            return { ...state, file: action.file };
        case actions.SET_INPUTS:
            return { ...state, inputs: action.inputs };
        case actions.SET_OUTPUTS:
            return { ...state, outputs: action.outputs };
        case actions.SET_METADATA_PROPS:
            return { ...state, metadataProps: action.metadataProps };
        case actions.SET_MODEL_INPUTS:
            return { ...state, modelInputs: action.modelInputs };
        case actions.SET_MODEL_OUTPUTS:
            return { ...state, modelOutputs: action.modelOutputs };
        case actions.SET_NODES:
            return { ...state, nodes: action.nodes };
        case actions.SET_PROPERTIES:
            return { ...state, properties: action.properties };
        case actions.SET_SAVE_FILE_NAME:
            return { ...state, saveFileName: action.saveFileName };
        case actions.SET_SELECTED_NODE:
            return { ...state, selectedNode: action.selectedNode };
        default:
            return state;
    }
}
