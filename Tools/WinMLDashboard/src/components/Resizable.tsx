import * as React from 'react';

import './Resizable.css';


interface IComponentProperties {
    isRightPanel?: boolean,
    visible?: boolean,
}

interface IComponentStates { 
    isResizing: boolean,
    resizeWidth: number,
};

export default class Resizable extends React.Component<IComponentProperties, IComponentStates> {
    public static defaultProps: Partial<IComponentProperties> = {
        isRightPanel: false,
        visible: false,
    };
    constructor(props: any) {
        super(props);
        this.state = {
            isResizing: false,
            resizeWidth: 350,
        }
    }
    public render() {
        // TODO Have a better resizable box (that can be resized by clicking and
        // draging the corners) instead of CSS' resize: horizontal property
        // TODO Have a direction (horizontal/vertical) option
        const style: React.CSSProperties = {};
        if (!this.props.visible) {
            style.width = style.minWidth = '0px';
            style.pointerEvents = 'none';
        }
        else {
            style.width = this.state.resizeWidth;
        }
        return (
            <div className="container" style={style} >
                <div className="drag" 
                    onMouseDown= {this.whenMouseDown}
                    />
                <div className='Resizable' >
                    {this.props.children}
                </div>
                <div className="drag" 
                    onMouseDown= {this.whenMouseDownRight} 
                    />  
            </div>
        );
        
    }
    private whenMouseDown = () => {
        this.setState({isResizing: true});
        window.addEventListener('mousemove', this.whenMouseMove, false);
        window.addEventListener('mouseup', this.whenMouseUp, false);
    }
    private whenMouseUp = () => {
        this.setState({isResizing: false});
        window.removeEventListener('mousemove', this.whenMouseMove, false);
        window.removeEventListener('mouseup', this.whenMouseUp, false);
    }
    private whenMouseMove = (e: MouseEvent) => {
        if(!(this.state.isResizing && this.props.isRightPanel)){
            return;
        }
        else {
            this.setState({resizeWidth: window.innerWidth - e.clientX});
        }
    }

    private whenMouseDownRight = () => {
        this.setState({isResizing: true});
        window.addEventListener('mousemove', this.whenMouseMoveRight, false);
        window.addEventListener('mouseup', this.whenMouseUpRight, false);
    }

    private whenMouseUpRight = () => {
        this.setState({isResizing: false});
        window.removeEventListener('mousemove', this.whenMouseMoveRight, false);
        window.removeEventListener('mouseup', this.whenMouseUpRight, false);
    }

    private whenMouseMoveRight = (e: MouseEvent) => {
        if(!(this.state.isResizing && !this.props.isRightPanel)){
            return;
        }
        else{
            this.setState({resizeWidth: e.clientX});
        }
    }
}
