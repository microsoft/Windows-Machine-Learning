import { IMetadataProps, IProperties } from './state';

export const SET_METADATA_PROPS = 'UPDATE_METADATA_PROPS';
export const SET_MODEL_INPUTS = 'UPDATE_INPUTS';
export const SET_MODEL_OUTPUTS = 'UPDATE_OUTPUTS';
export const SET_NODES = 'UPDATE_NODES';
export const SET_PROPERTIES = 'UPDATE_PROPERTIES';
export const SET_SELECTED_NODE = 'UPDATE_SELECTED_NODE';

interface IValueInfo {
    description?: string,
    id: string,
    name: string,
    type: string,
}

export interface IAction {
    type: string,
    inputs: IValueInfo,
    metadataProps: IMetadataProps,
    nodes: { [key: string]: any },
    outputs: IValueInfo,
    properties: IProperties,
    selectedNode: string,
}
