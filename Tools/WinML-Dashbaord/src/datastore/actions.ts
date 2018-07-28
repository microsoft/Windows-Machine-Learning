import { IMetadataProps, IProperties } from './state';

export const SET_INPUTS = 'SET_INPUTS';
export const SET_METADATA_PROPS = 'UPDATE_METADATA_PROPS';
export const SET_MODEL_INPUTS = 'UPDATE_MODEL_INPUTS';
export const SET_MODEL_OUTPUTS = 'UPDATE_MODEL_OUTPUTS';
export const SET_NODES = 'UPDATE_NODES';
export const SET_OUTPUTS = 'SET_OUTPUTS';
export const SET_PROPERTIES = 'UPDATE_PROPERTIES';
export const SET_SELECTED_NODE = 'UPDATE_SELECTED_NODE';

export interface IAction {
    nodes: { [key: string]: any },
    type: string,

    inputs: any[],
    outputs: any[],

    modelInputs: string[],
    modelOutputs: string[],

    metadataProps: IMetadataProps,
    properties: IProperties,
    selectedNode: string,
}
