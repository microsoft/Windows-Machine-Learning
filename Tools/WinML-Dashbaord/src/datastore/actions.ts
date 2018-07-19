import { IMetadataProps, IProperties } from './state';

export const UPDATE_GRAPH = 'UPDATE_GRAPH';
export const UPDATE_METADATA_PROPS = 'UPDATE_METADATA_PROPS';
export const UPDATE_PROPERTIES = 'UPDATE_PROPERTIES';

export interface IAction {
    type: string,
    graph: any,
    metadataProps: IMetadataProps,
    properties: IProperties,
}
