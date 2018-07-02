import React, { Component } from 'react';
import { Label } from 'office-ui-fabric-react/lib/Label';
import { Pivot, PivotItem } from 'office-ui-fabric-react/lib/Pivot';

import EditPage from './view/EditPage';

import './App.css';

class App extends Component {
  render() {
    return (
      <div className="App" style={{height: '100vh'}}>
        <Pivot>
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
}

export default App;
