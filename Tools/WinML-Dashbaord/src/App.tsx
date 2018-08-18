import { Pivot, PivotItem } from 'office-ui-fabric-react/lib/Pivot';
import * as React from 'react';

import { initializeIcons } from './fonts';
import ConvertView from './view/convert/View';
import EditView from './view/edit/View';
import LearnView from './view/learn/View';

import './App.css';

interface IComponentState {
    tab: string,
}

const views = {
    Edit: <EditView />,
    // tslint:disable-next-line:object-literal-sort-keys
    Convert: <ConvertView />,
    Learn: <LearnView />,
};

class App extends React.Component<{}, IComponentState> {
    constructor(props: {}) {
        super(props);
        initializeIcons();
        this.state = {
            tab: 'Edit',
        };
    }

    public render() {
        const pivotItems = Object.keys(views).map(x => <PivotItem headerText={x} key={x} />);
        const mainView = Object.entries(views).map(([k, v]) =>
            <div className='MainView' style={{ display: this.displayIfKeySelected(k) }} key={k} >
                {v}
            </div>
        )

        return (
            <div className='App'>
                <Pivot onLinkClick={this.onLinkClick}>
                    {pivotItems}
                </Pivot>
                {mainView}
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
