import * as React from 'react';

import './Resizable.css';

interface IComponentProperties {
    visible?: boolean,
}

export default class Resizable extends React.Component<IComponentProperties, {}> {
    public static defaultProps: Partial<IComponentProperties> = {
        visible: true,
    };

    public render() {
        // TODO Have a better resizable box (that can be resized by clicking and
        // draging the corners) instead of CSS' resize: horizontal property
        // TODO Have a direction (horizontal/vertical) option
        const style: React.CSSProperties = {};
        if (!this.props.visible) {
            style.width = style.minWidth = '0px';
            style.pointerEvents = 'none';
        }
        return (
            <div className='Resizable' style={style}>
                {this.props.children}
            </div>
        );
    }
}
