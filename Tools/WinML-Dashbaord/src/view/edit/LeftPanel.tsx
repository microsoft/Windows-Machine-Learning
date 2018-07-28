import { Label } from 'office-ui-fabric-react/lib/Label';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import * as React from 'react';
import { connect } from 'react-redux';

import Collapsible from '../../components/Collapsible';
import Resizable from '../../components/Resizable';
import IState from '../../datastore/state';

import './Panel.css';

interface IComponentProperties {
    // Redux properties
    nodes: { [key: string]: any },
    selectedNode: string,
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
        const node = this.props.nodes[this.props.selectedNode];
        const inputsForm = [];
        for (const input of node.input) {
            inputsForm.push(
                <div key={input}>
                    <Label className='TensorName'>{input}</Label>
                    <span className='Shape'>
                        <TextField inputMode='numeric' type='number' placeholder='N' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='C' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='H' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='W' className='ShapeTextField' />
                    </span>
                </div>
            );
        }
        const outputsForm = [];
        for (const output of node.output) {
            outputsForm.push(
                <div key={output}>
                    <Label className='TensorName'>{output}</Label>
                    <span className='Shape'>
                        <TextField inputMode='numeric' type='number' placeholder='N' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='C' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='H' className='ShapeTextField' />
                        <TextField inputMode='numeric' type='number' placeholder='W' className='ShapeTextField' />
                    </span>
                </div>
            );
        }
        return (
            <div>
                <Label className='PanelName'>{`Node: ${this.props.selectedNode || ''}`}</Label>
                <div className='Panel'>
                    <Collapsible label='Tensor shapes'>
                        <Label>Inputs</Label>
                        {inputsForm}
                        <Label>Outputs</Label>
                        {outputsForm}
                    </Collapsible>
                </div>
            </div>
        );
    }
}

const mapStateToProps = (state: IState) => ({
        nodes: state.nodes,
        selectedNode: state.selectedNode,
})

export default connect(mapStateToProps)(LeftPanel);
