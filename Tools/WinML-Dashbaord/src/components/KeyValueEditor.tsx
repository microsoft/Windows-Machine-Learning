import * as Ajv from 'ajv';
import { JsonEditor } from 'jsoneditor-react';
import 'jsoneditor-react/es/editor.min.css';
import * as React from 'react';
import { connect } from 'react-redux';

import IState from '../datastore/state';

const ajv = new Ajv({ allErrors: true, verbose: true });

interface IComponentProperties {
    actionCreator: (keyValueObject: { [key: string]: string }) => {},
    getState: (state: IState) => { [key: string]: string },
    schema: {},

    // Redux properties
    keyValueObject?: { [key: string]: string },
    updateKeyValueObject?: (keyValueObject: { [key: string]: string }) => void,
}

class KeyValueEditorComponent extends React.Component<IComponentProperties, {}> {
    public render() {
        return (
            <div key={JSON.stringify(this.props.keyValueObject)}>
                <JsonEditor
                    ajv={ajv}
                    schema={this.props.schema}
                    value={this.props.keyValueObject}
                    onChange={this.props.updateKeyValueObject}
                />
            </div>
        );
    }
}

const mapStateToProps = (state: IState, ownProps: IComponentProperties) => ({
    keyValueObject: ownProps.getState(state),
});

const mapDispatchToProps = (dispatch: any, ownProps: IComponentProperties) => ({
    updateKeyValueObject: (keyValueObject: { [key: string]: string }) => dispatch(ownProps.actionCreator(keyValueObject)),
})

const KeyValueEditor = connect(mapStateToProps, mapDispatchToProps)(KeyValueEditorComponent as any);
export default KeyValueEditor;
