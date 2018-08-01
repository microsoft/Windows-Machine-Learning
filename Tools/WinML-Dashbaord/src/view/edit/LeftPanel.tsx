import { ComboBox, IComboBoxOption } from 'office-ui-fabric-react/lib/ComboBox';
import { ExpandingCardMode, HoverCard, IExpandingCardProps } from 'office-ui-fabric-react/lib/HoverCard';
import { Label } from 'office-ui-fabric-react/lib/Label';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import * as React from 'react';
import { connect } from 'react-redux';

import Collapsible from '../../components/Collapsible';
import Resizable from '../../components/Resizable';
import { setInputs, setOutputs } from '../../datastore/actionCreators';
import IState from '../../datastore/state';

import './Panel.css';

interface IComponentProperties {
    // Redux properties
    inputs: { [key: string]: any },
    modelInputs: string[];
    modelOutputs: string[];
    nodes: { [key: string]: any },
    outputs: { [key: string]: any },
    selectedNode: string,
    setInputs: typeof setInputs,
    setOutputs: typeof setOutputs,
}

const tensorProtoDataType = [
    'UNDEFINED',
    'float',
    'uint8',
    'int8',
    'uint16',
    'int16',
    'int32',
    'int64',
    'string',
    'bool',
    'float16',
    'double',
    'uint32',
    'uint64',
    'complex64',
    'complex128',
]

const getFullType = (typeProto: any): string => {
    if (typeProto.tensorType) {
        return `Tensor<${tensorProtoDataType[typeProto.tensorType.elemType]}>`;
    } else if (typeProto.sequenceType) {
        return `List<${getFullType(typeProto.sequenceType.elemType)}>`;
    } else if (typeProto.mapType) {
        return `Map<${tensorProtoDataType[typeProto.mapType.keyType]}, ${getFullType(typeProto.mapType.valueType)}>`;
    }
    return 'unknown';
}

class LeftPanel extends React.Component<IComponentProperties, {}> {
    public render() {
        return (
            <Resizable visible={!!this.props.selectedNode}>
                {this.props.selectedNode && this.getContent()}
            </Resizable>
        );
    }

    private getContent() {
        const modelPropertiesSelected = this.props.selectedNode === 'Model Properties';
        let input: any[];
        let output: any[];
        if (modelPropertiesSelected) {
            input = this.props.modelInputs;
            output = this.props.modelOutputs;
        } else {
            ({ input, output } = this.props.nodes[this.props.selectedNode]);
        }

        const inputsForm = this.buildConnectionList(input);
        const outputsForm = this.buildConnectionList(output);
        return (
            <div>
                <Label className='PanelName'>{this.props.selectedNode ? (`${modelPropertiesSelected ? '' : 'Node: '}${this.props.selectedNode}`) : ''}</Label>
                <div className='Panel'>
                    <Collapsible label='Inputs'>
                        {inputsForm}
                    </Collapsible>
                    <Collapsible label='Outputs'>
                        {outputsForm}
                    </Collapsible>
                </div>
            </div>
        );
    }

    private buildConnectionList = (connections: any[]) => {
        const denotationOptions = ['', 'IMAGE', 'AUDIO', 'TEXT', 'TENSOR'].map((key: string) => ({ key, text: key }));
        return connections.reduce((acc: any[], x: any) => {
            const valueInfoProto = this.props.inputs[x] || this.props.outputs[x];
            if (!valueInfoProto) {
                acc.push(
                    <div key={x}>
                        <Label className='TensorName' disabled={true}>{x}</Label>
                    </div>
                );
                return acc;
            }

            const onnxType = Object.keys(valueInfoProto.type).find((t: string) => ['tensorType', 'sequenceType', 'mapType'].includes(t)) || 'unknownType';
            const type = onnxType.slice(0, -4);
            let tensorName = (
                <div className={valueInfoProto.docString ? 'TensorDocumentationHover' : 'TensorName'}>
                    <b>{x}</b><span>{` (type: ${getFullType(valueInfoProto.type)})`}</span>
                </div>
            );
            if (valueInfoProto.docString) {
                const expandingCardProps: IExpandingCardProps = {
                    compactCardHeight: 100,
                    mode: ExpandingCardMode.compact,
                    onRenderCompactCard: (item: any): JSX.Element => (
                        <p className='DocString'>{valueInfoProto.docString}</p>
                    ),
                };
                tensorName = (
                    <HoverCard expandingCardProps={expandingCardProps} instantOpenOnClick={true}>
                        {tensorName}
                    </HoverCard>
                );
            }

            const isModelInput = this.props.modelInputs.includes(x)
            const isModelOutput = this.props.modelOutputs.includes(x);
            let keyChangedCallback;
            if (isModelInput) {
                keyChangedCallback = (option?: IComboBoxOption, index?: number, value?: string) => {
                    const nextInputs = { ...this.props.inputs };
                    nextInputs[x].type.denotation = value || option!.text;
                    this.props.setInputs(nextInputs);
                };
            } else if (isModelOutput) {
                keyChangedCallback = (option?: IComboBoxOption, index?: number, value?: string) => {
                    const nextOutputs = { ...this.props.outputs };
                    nextOutputs[x].type.denotation = value || option!.text;
                    this.props.setOutputs(nextOutputs);
                };
            }

            let shapeEditor;
            if (type === 'tensor') {
                shapeEditor = (
                    <span className='Shape'>
                        <TextField inputMode='numeric' type='number' placeholder='N' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='C' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='H' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='W' className='ShapeTextField' />
                    </span>
                )
            }

            acc.push(
                <div key={x}>
                    {tensorName}
                    <div className='TensorTypeDenotationDiv'>
                        <Label className='TensorTypeDenotationLabel'>Type denotation</Label>
                        <ComboBox
                            className='TensorTypeDenotation'
                            placeholder='Type denotation'
                            allowFreeform={true}
                            text={valueInfoProto.type.denotation}
                            options={denotationOptions}
                            disabled={!isModelInput && !isModelOutput}
                            onChanged={keyChangedCallback}
                        />
                    </div>
                    {shapeEditor}
                </div>
            );
            return acc;
        }, []);
    }
}

const mapStateToProps = (state: IState) => ({
    inputs: state.inputs,
    modelInputs: state.modelInputs,
    modelOutputs: state.modelOutputs,
    nodes: state.nodes,
    outputs: state.outputs,
    selectedNode: state.selectedNode,
})

const mapDispatchToProps = {
    setInputs,
    setOutputs,
};

export default connect(mapStateToProps, mapDispatchToProps)(LeftPanel);
