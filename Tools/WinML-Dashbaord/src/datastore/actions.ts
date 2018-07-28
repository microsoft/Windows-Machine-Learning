import { IMetadataProps, IProperties } from './state';

export const UPDATE_INPUTS = 'UPDATE_INPUTS';
export const UPDATE_METADATA_PROPS = 'UPDATE_METADATA_PROPS';
export const UPDATE_NODES = 'UPDATE_NODES';
export const UPDATE_OUTPUTS = 'UPDATE_OUTPUTS';
export const UPDATE_PROPERTIES = 'UPDATE_PROPERTIES';
export const UPDATE_SELECTED_NODE = 'UPDATE_SELECTED_NODE';

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
    nodes: any,
    outputs: IValueInfo,
    properties: IProperties,
    selectedNode: string,
}
