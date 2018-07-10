import { Label } from 'office-ui-fabric-react/lib/Label';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import * as React from 'react';

import Collapsible from '../../components/Collapsible';

import './Panel.css';

export default class LeftPanel extends React.Component {
    public render() {
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
                <Label className='PanelName'>Layer</Label>
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
