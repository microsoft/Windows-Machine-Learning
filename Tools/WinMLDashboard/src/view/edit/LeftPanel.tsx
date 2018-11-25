import { ComboBox, IComboBoxOption } from 'office-ui-fabric-react/lib/ComboBox';
import { ExpandingCardMode, HoverCard, IExpandingCardProps } from 'office-ui-fabric-react/lib/HoverCard';
import { Label } from 'office-ui-fabric-react/lib/Label';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import * as React from 'react';
import { connect } from 'react-redux';

import Collapsible from '../../components/Collapsible';
import Resizable from '../../components/Resizable';
import { setInputs, setOutputs, setShowLeft } from '../../datastore/actionCreators';
import { Proto } from '../../datastore/proto/proto';
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
    setShowLeft: typeof setShowLeft,
    showLeft: boolean,
}

const denotationOptions = ['', 'IMAGE', 'AUDIO', 'TEXT', 'TENSOR'].map((key: string) => ({ key, text: key }));
const dimensionDenotationOptions = [
    'DATA_BATCH',
    'DATA_CHANNEL',
    'DATA_TIME',
    'DATA_FEATURE',
    'FILTER_IN_CHANNEL',
    'FILTER_OUT_CHANNEL',
    'FILTER_SPATIAL',
].map((key) => ({ key, text: key }));

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
            <div className="Unselectable">
                <Resizable visible={this.props.showLeft}>
                    {this.props.showLeft && this.getContent()}
                </Resizable>
            </div>
        );
    }

    private getContent() {
        const name = 'Input And Output';

        const input = this.props.modelInputs;
        const output = this.props.modelOutputs;
        const inputsForm = this.buildConnectionList(input);
        const outputsForm = this.buildConnectionList(output);
        return (
            <div>
                <Label className='PanelName' onClick={this.toggleLeft}>{name}</Label>
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
    private toggleLeft = ()=> {
        this.props.setShowLeft(false)
    }
    private buildConnectionList = (connections: any[]) => {
        return connections.map((x: any) => {
            const valueInfoProto = this.props.inputs[x] || this.props.outputs[x];
            if (!valueInfoProto) {
                return (
                    <div key={x}>
                        <Label className='TensorName' disabled={true}>{`${x} (type: internal connection)`}</Label>
                    </div>
                );
            }

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
            const disabled = !isModelInput && !isModelOutput;
            const valueInfoProtoCopy = () => Proto.types.ValueInfoProto.fromObject(Proto.types.ValueInfoProto.toObject(valueInfoProto));
            const tensorDenotationChanged = (option?: IComboBoxOption, index?: number, value?: string) => {
                const nextValueInfoProto = valueInfoProtoCopy();
                nextValueInfoProto.type.denotation = value || option!.text;
                this.props[isModelInput ? 'setInputs' : 'setOutputs']({
                    ...this.props[isModelInput ? 'inputs' : 'outputs'],
                    [x]: nextValueInfoProto,
                });
            };

            let shapeEditor;
            if (valueInfoProto.type.tensorType) {
                const getValueInfoDimensions = (valueInfo: any) => valueInfo.type.tensorType.shape.dim;
                shapeEditor = getValueInfoDimensions(valueInfoProto).map((dim: any, index: number) => {
                    const dimensionChanged = (value: string) => {
                        const nextValueInfoProto = valueInfoProtoCopy();
                        const dimension = getValueInfoDimensions(nextValueInfoProto)[index]
                        if (value) {
                            dimension.dimValue = +value;
                            delete dimension.dimParam;
                        } else {
                            dimension.dimParam = 'None'
                            delete dimension.dimValue;
                        }
                        this.props[isModelInput ? 'setInputs' : 'setOutputs']({
                            ...this.props[isModelInput ? 'inputs' : 'outputs'],
                            [x]: nextValueInfoProto,
                        });
                    }
                    const dimensionDenotationChanged = (option?: IComboBoxOption) => {
                        const nextValueInfoProto = valueInfoProtoCopy();
                        getValueInfoDimensions(nextValueInfoProto)[index].denotation = option!.text;
                        this.props[isModelInput ? 'setInputs' : 'setOutputs']({
                            ...this.props[isModelInput ? 'inputs' : 'outputs'],
                            [x]: nextValueInfoProto,
                        });
                    };
                    return (
                        <div key={index}>
                            <div className='DisplayFlex'>
                                <TextField
                                    className='DenotationLabel'
                                    label={`Dimension [${index}]`}
                                    value={dim.dimParam === 'None' ? undefined : dim.dimValue}
                                    inputMode='numeric'
                                    type='number'
                                    placeholder='None'
                                    disabled={disabled}
                                    onChanged={dimensionChanged} />
                                <ComboBox
                                    label='Denotation'
                                    defaultSelectedKey={dim.denotation}
                                    className='DenotationComboBox'
                                    options={dimensionDenotationOptions}
                                    disabled={disabled}
                                    onChanged={dimensionDenotationChanged} />
                            </div>
                        </div>
                    );
                });
            }

            return (
                <div key={x}>
                    {tensorName}
                    <div className='DenotationDiv'>
                        <ComboBox
                            className='DenotationComboBox'
                            label='Type denotation'
                            allowFreeform={true}
                            text={valueInfoProto.type.denotation}
                            options={denotationOptions}
                            disabled={!isModelInput && !isModelOutput}
                            onChanged={tensorDenotationChanged} />
                    </div>
                    {shapeEditor}
                </div>
            );
        });
    }
}

const mapStateToProps = (state: IState) => ({
    inputs: state.inputs,
    modelInputs: state.modelInputs,
    modelOutputs: state.modelOutputs,
    nodes: state.nodes,
    outputs: state.outputs,
    selectedNode: state.selectedNode,
    showLeft: state.showLeft,
})

const mapDispatchToProps = {
    setInputs,
    setOutputs,
    setShowLeft,
};

export default connect(mapStateToProps, mapDispatchToProps)(LeftPanel);
