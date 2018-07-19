import { Label } from 'office-ui-fabric-react/lib/Label';
import { Pivot, PivotItem } from 'office-ui-fabric-react/lib/Pivot';
import * as React from 'react';

import DocsView from './learn/Learn';
import EditView from './view/edit/View';

import './App.css';

class App extends React.Component<any, any> {
  public render() {
    return (
      <div className='App'>
        <Pivot>
          <PivotItem headerText='Edit'>
            <EditView />
          </PivotItem>
          <PivotItem headerText='Convert'>
            <Label>TODO</Label>
          </PivotItem>
          <PivotItem headerText='Debug'>
            <Label>TODO</Label>
          </PivotItem>
          <PivotItem headerText='Profile'>
            <Label>TODO</Label>
          </PivotItem>
          <PivotItem headerText='Learn'>
              <DocsView />
          </PivotItem>
        </Pivot>
      </div>
    );
  }
}

export default App;
