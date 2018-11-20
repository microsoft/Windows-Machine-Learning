import * as React from 'react';

import LeftPanel from './LeftPanel';
import Netron from './netron/Netron';
import RightPanel from './RightPanel';

import './View.css';

import log from 'electron-log';

export default class EditView extends React.Component {
    constructor(props: any) {
        super(props);
        log.info("Edit view is created.");
    }
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
