import * as React from 'react';
import { Label } from 'office-ui-fabric-react/lib/Label';
import './EditPage.css';

export default class EditPage extends React.Component {
  render() {
    return (
      <div id='EditPage'>
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

  componentDidMount() {
    const page = document.getElementById('EditPage');
    page && page.scrollIntoView({
      behavior: 'smooth',
    });
  }
}
