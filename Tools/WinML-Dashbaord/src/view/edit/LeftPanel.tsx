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
    graph?: any,
}

class LeftPanel extends React.Component<IComponentProperties, {}> {
    public render() {
        return (
            <Resizable visible={!!this.props.graph}>
                {this.getContent()}
            </Resizable>
        );
    }

    private getContent() {
        const inputs = [];
        inputs.push('stub');
        const inputsForm = [];
        for (const _ of inputs) {
            inputsForm.push(<div key={0}>
                <Label className='TensorName'>data_0</Label>
                <span className='Shape'>
                    <TextField inputMode='numeric' type='number' placeholder='N' className='ShapeTextField' />
                    <TextField inputMode='numeric' type='number' placeholder='C' className='ShapeTextField' />
                    <TextField inputMode='numeric' type='number' placeholder='H' className='ShapeTextField' />
                    <TextField inputMode='numeric' type='number' placeholder='W' className='ShapeTextField' />
                </span>
            </div>);
        }
        return (
            <div>
                <Label className='PanelName'>Node</Label>
                <div className='Panel'>
                    <Collapsible label='Tensor shapes'>
                        <Label>Inputs</Label>
                        {inputsForm}
                        <Label>Outputs</Label>
                    </Collapsible>
                </div>
            </div>
        );
    }
}

const mapStateToProps = (state: IState) => ({
        graph: state.graph,
})

export default connect(mapStateToProps)(LeftPanel);
