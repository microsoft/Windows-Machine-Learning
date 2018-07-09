import * as React from 'react';

import Resizable from '../../components/Resizable';
import LeftPanel from './LeftPanel';
import RightPanel from './RightPanel';

import './View.css';

export default class EditView extends React.Component {
    private tab: React.RefObject<HTMLDivElement>;

    constructor(props: {}) {
        super(props);
        this.tab = React.createRef();
    }

    public render() {
        return (
            <div id='EditView' ref={this.tab}>
                <Resizable>
                    <LeftPanel />
                </Resizable>
                <object className='Netron' type='text/html' data='static/Netron/'>
                    Netron visualization
                </object>
                <Resizable>
                    <RightPanel />
                </Resizable>
            </div>
      );
  }

    public componentDidMount() {
        if (this.tab.current) {
            this.tab.current.scrollIntoView({
                behavior: 'smooth',
            });
        }
    }
}
