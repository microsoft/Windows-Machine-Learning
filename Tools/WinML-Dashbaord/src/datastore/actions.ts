import { IMetadataProps, IProperties } from './state';

export const UPDATE_GRAPH = 'UPDATE_GRAPH';
export const UPDATE_INPUTS = 'UPDATE_INPUTS';
export const UPDATE_METADATA_PROPS = 'UPDATE_METADATA_PROPS';
export const UPDATE_PROPERTIES = 'UPDATE_PROPERTIES';

export interface IAction {
    type: string,
    graph: any,
    inputs: [{description: string, id: string, name: string, type: string}],
    metadataProps: IMetadataProps,
    properties: IProperties,
}
