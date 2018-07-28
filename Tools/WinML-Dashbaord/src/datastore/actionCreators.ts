import * as actions from './actions';
import { IMetadataProps, IProperties } from './state';

export const updateInputs = (inputs: any) => ({
    inputs,
    type: actions.UPDATE_INPUTS,
})

export const updateOutputs = (outputs: any) => ({
    outputs,
    type: actions.UPDATE_OUTPUTS,
})

export const updateMetadataProps = (metadataProps: IMetadataProps) => ({
    metadataProps,
    type: actions.UPDATE_METADATA_PROPS,
})

export const updateNodes = (nodes: any) => ({
    nodes,
    type: actions.UPDATE_NODES,
})

export const updateProperties = (properties: IProperties) => ({
    properties,
    type: actions.UPDATE_PROPERTIES,
})

export const updateSelectedNode = (selectedNode?: string) => ({
    selectedNode,
    type: actions.UPDATE_SELECTED_NODE,
})
