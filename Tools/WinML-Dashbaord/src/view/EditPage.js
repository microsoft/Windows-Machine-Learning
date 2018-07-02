import React, { Component } from 'react';
import { Label } from 'office-ui-fabric-react/lib/Label';
import './EditPage.css';

export default class EditPage extends Component {
  render() {
    return (
      <div className='EditPage'>
        <div className='Panel'>
          <Label>Left panel</Label>
        </div>
        <object className='Netron' type='text/html' data='static/Netron/'>
            Netron visualization
        </object>
        <div className='Panel'>
          <Label>Right panel</Label>
        </div>
      </div>
    );
  }
}
