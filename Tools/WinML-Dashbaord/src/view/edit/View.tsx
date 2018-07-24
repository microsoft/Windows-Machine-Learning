import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import * as React from 'react';

import Resizable from '../../components/Resizable';
import { ModelProto } from '../../datastore/proto/modelProto';
import store from '../../datastore/store';
import LeftPanel from './LeftPanel';
import * as netron from './netron/Netron';
import RightPanel from './RightPanel';

import './View.css';

interface IComponentState {
    file?: File,
}

export default class EditView extends React.Component<{}, IComponentState> {
    private tab: React.RefObject<HTMLDivElement> = React.createRef();
    private openFileInput: React.RefObject<HTMLInputElement> = React.createRef();
    private anchor: React.RefObject<HTMLAnchorElement> = React.createRef();

    constructor(props: {}) {
        super(props);
        this.state = {
            file: undefined,
        };
    }

    public render() {
        return (
            <div id='EditView' ref={this.tab}>
                <Resizable>
                    <LeftPanel />
                </Resizable>
                <div className='Netron'>
                    <netron.Netron file={this.state.file} />
                </div>
                <Resizable>
                    <DefaultButton text='Open file' onClick={this.openFile}/>
                    <DefaultButton text='Save file' onClick={this.saveFile}/>
                    <a ref={this.anchor} style={{display: 'none'}} />
                    <input type='file' style={{display: 'none'}} accept=".onnx,.pb,.meta,.tflite,.keras,.h5,.json,.mlmodel,.caffemodel" ref={this.openFileInput} />
                    <RightPanel />
                </Resizable>
            </div>
        );
    }

    public componentDidMount() {
        if (this.tab.current) {
            this.tab.current.scrollIntoView({
                behavior: 'smooth',
            });
        }
    }

    private openFile = () => {
        if (!this.openFileInput.current) {
            return;
        }
        this.openFileInput.current.addEventListener('change', this.onFileSelected);
        this.openFileInput.current.click();
    }

    private onFileSelected = () => {
        const openFileInput = this.openFileInput.current;
        if (!openFileInput) {
            return;
        }
        const files = openFileInput.files;
        if (!files || !files.length) {
            return;
        }
        this.setState({
            file: files[0],
        })
    }

    private saveFile = () => {
        const anchor = this.anchor.current;
        if (!anchor) {
            return;
        }
        if (anchor.href) {
            // Release previous object URL
            URL.revokeObjectURL(anchor.href);
        }
        // TODO Move these to the ModelProto class?
        // TODO Refactor data store access
        ModelProto.setMetadata(store.getState().metadataProps);
        const blob = new Blob([ModelProto.serialize()], {type: 'application/octet-stream'});
        anchor.href = URL.createObjectURL(blob);
        anchor.download = 'model.onnx';
        anchor.click();
    }
}
