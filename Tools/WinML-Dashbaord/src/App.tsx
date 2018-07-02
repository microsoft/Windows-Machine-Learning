import * as React from 'react';
import { Label } from 'office-ui-fabric-react/lib/Label';
import { Pivot, PivotItem } from 'office-ui-fabric-react/lib/Pivot';

import EditPage from './view/EditPage';

import './App.css';

class App extends React.Component<any, any> {
  render() {
    return (
      <div className="App">
        <Pivot onLinkClick={(_: PivotItem) => {window.scrollTo(0,document.body.scrollHeight)}}>
          <PivotItem headerText="Edit" style={{height: '100vh'}}>
            <EditPage />
          </PivotItem>
          <PivotItem headerText="Convert">
            <Label>TODO</Label>
          </PivotItem>
          <PivotItem headerText="Debug">
            <Label>TODO</Label>
          </PivotItem>
          <PivotItem headerText="Profile">
            <Label>TODO</Label>
          </PivotItem>
        </Pivot>
      </div>
    );
  }

  private _handleLinkClick = (item: PivotItem): void => {
    this.setState({
      selectedKey: item.props.itemKey
    });
  };
}

export default App;
