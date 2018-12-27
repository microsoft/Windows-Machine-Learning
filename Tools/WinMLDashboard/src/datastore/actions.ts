import { DebugFormat, IMetadataProps, IProperties } from './state';

export const SET_SHOWLEFT = 'SET_SHOWLEFT';
export const SET_SHOWRIGHT = 'SET_SHOWRIGHT';
export const SET_FILE = 'SET_FILE';
export const SET_DEBUG_NODES = 'SET_DEBUG_NODES';
export const SET_INPUTS = 'SET_INPUTS';
export const SET_INTERMEDIATE_OUTPUTS = 'SET_INTERMEDIATE_OUTPUTS';
export const SET_METADATA_PROPS = 'SET_METADATA_PROPS';
export const SET_MODEL_INPUTS = 'SET_MODEL_INPUTS';
export const SET_MODEL_OUTPUTS = 'SET_MODEL_OUTPUTS';
export const SET_NODES = 'SET_NODES';
export const SET_OUTPUTS = 'SET_OUTPUTS';
export const SET_PROPERTIES = 'SET_PROPERTIES';
export const SET_SAVE_FILE_NAME = 'SET_SAVE_FILE_NAME';
export const SET_SELECTED_NODE = 'SET_SELECTED_NODE';

export interface IAction {
    showLeft: boolean,
    showRight: boolean,

    file: File,
    nodes: { [key: string]: any },
    debugNodes: { [output: string]: DebugFormat[] } ,
    saveFileName: string,
    type: string,

    inputs: any[],
    outputs: any[],

    modelInputs: string[],
    modelOutputs: string[],

    metadataProps: IMetadataProps,
    properties: IProperties,
    selectedNode: string,
}
