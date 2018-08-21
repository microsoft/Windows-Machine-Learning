import * as React from 'react';

import LeftPanel from './LeftPanel';
import { Netron } from './netron/Netron';
import RightPanel from './RightPanel';

import './View.css';

export default class EditView extends React.Component {
    public render() {
        return (
            <div id='EditView'>
                <LeftPanel />
                <Netron />
                <RightPanel />
            </div>
        );
    }
}
