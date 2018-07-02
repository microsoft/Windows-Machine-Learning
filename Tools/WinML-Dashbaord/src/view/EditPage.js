import React, { Component } from 'react';
import './EditPage.css';

export default class EditPage extends Component {
  render() {
    return (
      <div class="EditPage" style={{height: '100vh'}}>
        <object class="Netron" type="text/html" data="static/Netron/" width="100%" height="100%">
            Netron visualization
        </object>
      </div>
    );
  }
}
