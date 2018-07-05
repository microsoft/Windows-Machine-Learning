import * as React from 'react';

import './Resizable.css';

type ComponentProperties = {}
type ComponentState = {}

export default class Resizable extends React.Component<ComponentProperties, ComponentState> {
    render() {
        // TODO Have a better resizable box (that can be resized by clicking and
        // draging the corners) instead of CSS' resize: horizontal property
        // TODO Have a direction (horizontal/vertical) option
        return (
            <div className='Resizable'>
                {this.props.children}
            </div>
        );
    }
}
