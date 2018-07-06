import { Label } from 'office-ui-fabric-react/lib/Label';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import * as React from 'react';

import Collapsible from '../../components/Collapsible';

import './Panel.css';

export default class LeftPanel extends React.Component {
    render() {
        const inputs = [], outputs = [];
        inputs.push('hi');
        outputs.push('hi');
        const inputsForm = [];
        // for (const input of inputs) {
        for (const _ of inputs) {
            inputsForm.push(<div>
                <TextField placeholder='N' className='ShapeTextField'>
                </TextField>
                <TextField placeholder='C' className='ShapeTextField'>
                </TextField>
                <TextField placeholder='H' className='ShapeTextField'>
                </TextField>
                <TextField placeholder='W' className='ShapeTextField'>
                </TextField>
            </div>);
        }
        return (
            <div className='Panel'>
                <Label className='PanelName'>Layer</Label>
                <Collapsible label='Tensor shapes'>
                    <Label>Inputs</Label>
                    {inputsForm}
                    <Label>Outputs</Label>
                </Collapsible>
            </div>
        );
    }
}
