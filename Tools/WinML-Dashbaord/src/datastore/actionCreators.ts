import * as actions from './actions';
import { IMetadataProps, IProperties } from './state';

export const setMetadataProps = (metadataProps: IMetadataProps) => ({
    metadataProps,
    type: actions.SET_METADATA_PROPS,
})

export const setModelInputs = (inputs: any) => ({
    inputs,
    type: actions.SET_MODEL_INPUTS,
})

export const setModelOutputs = (outputs: any) => ({
    outputs,
    type: actions.SET_MODEL_OUTPUTS,
})

export const setNodes = (nodes?: { [key: string]: any }) => ({
    nodes,
    type: actions.SET_NODES,
})

export const setProperties = (properties: IProperties) => ({
    properties,
    type: actions.SET_PROPERTIES,
})

export const setSelectedNode = (selectedNode?: string) => ({
    selectedNode,
    type: actions.SET_SELECTED_NODE,
})
