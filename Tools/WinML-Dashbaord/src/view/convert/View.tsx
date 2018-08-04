// tslint:disable:no-console
import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ChoiceGroup } from 'office-ui-fabric-react/lib/ChoiceGroup';
import { MessageBar, MessageBarType } from 'office-ui-fabric-react/lib/MessageBar';
import * as React from 'react';

import { winmlDataFoler } from '../../persistence/appData';
import { getPythonBinaries, getVenvPython } from '../../python/python';

import './View.css';

export default class ConvertView extends React.Component {
    public render() {
        return (
            <div className='ConvertView'>
                {this.pythonChooser()}
            </div>
        )
    }

    private pythonChooser() {
        if (winmlDataFoler === '/') {
            return (
                <MessageBar messageBarType={MessageBarType.error}>
                    The converter can't be run in the web interface
                </MessageBar>
            );
        }
        if (!getVenvPython()) {
            // const binaries = getPythonBinaries();
            getPythonBinaries();
            // console.log(binaries);
            const binaries = ['a', 'c', 'fsafsad', null];
            const options = binaries.map((key) => key ? { key, text: key } : { key: '__download', text: 'Download a new Python binary to be used exclusively by the WinML Dashboard' });
            return (
                <ChoiceGroup
                    options={options}
                    label={binaries === [null] ? 'No suitable Python versions were found in the system.' : 'Suitable Python versions were found in the system. Pick one to be used by the converter.'}
                />
            );
        }
        return <DefaultButton style={{ height: '100vh', width: '100%' }} text='Open file' onClick={this.openFile}/>;
    }

    private openFile = () => {
        console.log('Convert')
    }
}
