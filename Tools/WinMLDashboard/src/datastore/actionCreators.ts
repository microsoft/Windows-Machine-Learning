import * as actions from './actions';
import { IDebugNodeMap, IMetadataProps, IProperties } from './state';

export const setShowLeft = (showLeft: boolean) => ({
    showLeft,
    type: actions.SET_SHOWLEFT,
})

export const setShowRight = (showRight: boolean) => ({
    showRight,
    type: actions.SET_SHOWRIGHT,
})

export const setFile = (file?: File) => ({
    file,
    type: actions.SET_FILE,
})

export const setDebugNodes = (debugNodes?: IDebugNodeMap) => ({
    debugNodes,
    type: actions.SET_DEBUG_NODES,
})

export const setInputs = (inputs?: { [key: string]: any }) => ({
    inputs,
    type: actions.SET_INPUTS,
})

export const setMetadataProps = (metadataProps: IMetadataProps) => ({
    metadataProps,
    type: actions.SET_METADATA_PROPS,
})

export const setModelInputs = (modelInputs?: string[]) => ({
    modelInputs,
    type: actions.SET_MODEL_INPUTS,
})

export const setModelOutputs = (modelOutputs?: string[]) => ({
    modelOutputs,
    type: actions.SET_MODEL_OUTPUTS,
})

export const setNodes = (nodes?: { [key: string]: any }) => ({
    nodes,
    type: actions.SET_NODES,
})

export const setOutputs = (outputs?: { [key: string]: any }) => ({
    outputs,
    type: actions.SET_OUTPUTS,
})

export const setProperties = (properties: IProperties) => ({
    properties,
    type: actions.SET_PROPERTIES,
})

export const setSaveFileName = (saveFileName?: string) => ({
    saveFileName,
    type: actions.SET_SAVE_FILE_NAME,
})

export const setSelectedNode = (selectedNode?: string) => ({
    selectedNode,
    type: actions.SET_SELECTED_NODE,
})
