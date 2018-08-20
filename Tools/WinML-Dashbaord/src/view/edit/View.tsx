import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import * as React from 'react';

import Resizable from '../../components/Resizable';
import { ModelProtoSingleton } from '../../datastore/proto/modelProto';
import { showWebOpenDialog, showWebSaveDialog } from '../../native/dialog';
import LeftPanel from './LeftPanel';
import * as netron from './netron/Netron';
import RightPanel from './RightPanel';

import './View.css';

interface IComponentState {
    file?: File,
}

export default class EditView extends React.Component<{}, IComponentState> {
    constructor(props: {}) {
        super(props);
        this.state = {
            file: undefined,
        };
    }

    public render() {
        return (
            <div id='EditView'>
                <LeftPanel />
                <div className='Netron'>
                    <netron.Netron file={this.state.file} />
                </div>
                <Resizable>
                    <DefaultButton text='Open file' onClick={this.openFile}/>
                    <DefaultButton text='Save file' onClick={this.saveFile}/>
                    <RightPanel />
                </Resizable>
            </div>
        );
    }

    private openFile = () =>
        showWebOpenDialog('.onnx,.pb,.meta,.tflite,.keras,.h5,.json,.mlmodel,.caffemodel')
            .then((files) => {
                if (files) {
                    this.setState({ file: files[0] });
                }
            });

    private saveFile = () => showWebSaveDialog(ModelProtoSingleton.serialize(), 'model.onnx');
}
