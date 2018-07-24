import { Label } from 'office-ui-fabric-react/lib/Label';
import { Pivot, PivotItem } from 'office-ui-fabric-react/lib/Pivot';
import * as React from 'react';

import { initializeIcons } from './fonts';
import EditView from './view/edit/View';
import LearnView from './view/learn/View';

import './App.css';

interface IComponentState {
    tab: string,
}

class App extends React.Component<{}, IComponentState> {
    constructor(props: {}) {
        super(props);
        initializeIcons();
        this.state = {
            tab: 'Edit',
        };
    }

    public render() {
        return (
            <div className='App'>
                <Pivot onLinkClick={this.onLinkClick}>
                    <PivotItem headerText='Edit' />
                    <PivotItem headerText='Convert' />
                    <PivotItem headerText='Debug' />
                    <PivotItem headerText='Profile' />
                    <PivotItem headerText='Learn' />
                </Pivot>
                <div style={{ display: this.displayIfKeySelected('Edit') }} >
                    <EditView />
                </div>
                <div style={{ display: this.displayIfKeySelected('Convert') }}>
                    <Label>TODO</Label>
                </div>
                <div style={{ display: this.displayIfKeySelected('Debug') }}>
                    <Label>TODO</Label>
                </div>
                <div style={{ display: this.displayIfKeySelected('Profile') }}>
                    <Label>TODO</Label>
                </div>
                <div style={{ display: this.displayIfKeySelected('Learn') }}>
                    <LearnView />
                </div>
            </div>
        );
    }

    private onLinkClick = (item?: PivotItem, ev?: React.MouseEvent<HTMLElement>) => {
        if (item) {
            this.setState({ tab: item.props.headerText! });
        }
    }

    private displayIfKeySelected = (key: string) => {
        return key === this.state.tab ? 'block' : 'none';
    }
}

export default App;
