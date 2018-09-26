import { Label } from 'office-ui-fabric-react/lib/Label';
import * as React from 'react';

import './Collapsible.css';

interface IComponentProperties {
    label: string,
}
interface IComponentState {
    visible: boolean,
}

export default class Collapsible extends React.Component<IComponentProperties, IComponentState> {
    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            visible: true,
        };
    }

    public render() {
        const style: React.CSSProperties = {};
        if (!this.state.visible) {
            style.maxHeight = '0px';
            style.visibility = 'hidden';
            style.padding = '0px';
        }
        return (
            <div>
                <Label className='Label' onClick={this.toggle}><pre>{this.state.visible && '-' || '+'}</pre>{this.props.label}</Label>
                <div className='Content' style={style}>
                    {this.props.children}
                </div>
            </div>
        );
    }

    private toggle = () => {
        this.setState(previousState => ({
            visible: !previousState.visible,
        }));
    }
}
