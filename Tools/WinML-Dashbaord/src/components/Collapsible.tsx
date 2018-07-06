import { Label } from 'office-ui-fabric-react/lib/Label';
import * as React from 'react';

import './Collapsible.css'

type ComponentProperties = {
    label: string,
}
type ComponentState = {
    visible: boolean,
}

export default class Collapsible extends React.Component<ComponentProperties, ComponentState> {
    constructor(props: ComponentProperties) {
        super(props);
        this.state = {
            visible: true,
        };
    }

    render() {
        return (
            <div className='Collapsible'>
                <Label className='Label' onClick={this.toggle}><pre>{this.state.visible && '-' || '+'}</pre>{this.props.label}</Label>
                <div className='Content' style={{display: this.state.visible && 'block' || 'none'}}>
                    {this.props.children}
                </div>
            </div>
        );
    }

    toggle = () => {
        this.setState(previousState => ({
            visible: !previousState.visible,
        }));
    }
}
