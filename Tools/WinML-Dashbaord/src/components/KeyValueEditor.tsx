// import * as Ajv from 'ajv';
import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ComboBox, IComboBoxOption } from 'office-ui-fabric-react/lib/ComboBox';
import * as React from 'react';
import { connect } from 'react-redux';

import IState from '../datastore/state';

import './KeyValueEditor.css';

// const ajv = new Ajv({ allErrors: true, verbose: true });

interface IComponentProperties {
    actionCreator: (keyValueObject: { [key: string]: string }) => {},
    getState: (state: IState) => { [key: string]: string },
    schema: any,

    // Redux properties
    keyValueObject?: { [key: string]: string },
    updateKeyValueObject?: (keyValueObject: { [key: string]: string }) => void,
}

class KeyValueEditorComponent extends React.Component<IComponentProperties, {}> {
    public render() {
        let rows = [];
        if (this.props.keyValueObject) {
            const schemaEntries = Object.entries(this.props.schema.properties);
            const knownKeys = Object.keys(this.props.schema.properties).filter(
                // Don't suggesting adding keys that already exist
                (x) => Object.keys(this.props.keyValueObject!).every((key) => !this.areSameKeys(key, x)));
            const options = knownKeys.map((key: string) => ({ key, text: key }));

            rows = Object.keys(this.props.keyValueObject!).reduce((acc: any[], x: string) => {
                const keyChangedCallback = (option?: IComboBoxOption, index?: number, value?: string) => {
                    const key = value || option!.text;
                    const newObject = { ...this.props.keyValueObject! };
                    newObject[key] = newObject[x];
                    delete newObject[x];
                    this.props.updateKeyValueObject!(newObject);
                };
                const valueChangedCallback = (option?: IComboBoxOption, index?: number, value?: string) => {
                    const newValue = value || option!.text;
                    this.props.updateKeyValueObject!({ ...this.props.keyValueObject!, [x]: newValue });
                };

                let knownValues = [];
                const property = schemaEntries.find((prop) => prop[0].toLowerCase() === x.toLowerCase()) as [string, any];
                if (property && property[1].enum) {
                   knownValues = property[1].enum.map((key: string) => ({ key, text: key }));
                }

                acc.push(
                    <div key={x} id='KeyValueItem'>
                        <ComboBox
                            className='KeyValueBox'
                            allowFreeform={true}
                            text={x}
                            options={options}
                            onChanged={keyChangedCallback}
                        />
                        <span className='KeyValueSeparator'>=</span>
                        <ComboBox
                            className='KeyValueBox'
                            allowFreeform={true}
                            text={this.props.keyValueObject![x]}
                            options={knownValues}
                            onChanged={valueChangedCallback}
                        />
                    </div>
                );
                return acc;
            }, []);
        }

        const addDisabled = !(this.props.keyValueObject && Object.keys(this.props.keyValueObject).every((x) => !!x))
        const removeDisabled = !(this.props.keyValueObject && Object.keys(this.props.keyValueObject).length)

        return (
            <div>
                {rows}
                <DefaultButton className='KeyValueEditorButton' iconProps={{ iconName: 'Add' }} disabled={addDisabled} onClick={this.addMetadataProp} />
                <DefaultButton className='KeyValueEditorButton' iconProps={{ iconName: 'Cancel' }} disabled={removeDisabled} onClick={this.removeMetadataProp} />
            </div>
        );
    }

    private areSameKeys = (a: string, b: string) => {
        return a.toLowerCase() === b.toLowerCase();
    }

    private addMetadataProp = () => {
        if (this.props.keyValueObject) {
            this.props.updateKeyValueObject!({ ...this.props.keyValueObject, '': ''});
        }
    };

    private removeMetadataProp = () => {
        const newObject = { ...this.props.keyValueObject! };
        const keys = Object.keys(newObject);
        delete newObject[keys[keys.length - 1]];
        this.props.updateKeyValueObject!(newObject);
    };
}

const mapStateToProps = (state: IState, ownProps: IComponentProperties) => ({
    keyValueObject: ownProps.getState(state),
});

const mapDispatchToProps = (dispatch: any, ownProps: IComponentProperties) => ({
    updateKeyValueObject: (keyValueObject: { [key: string]: string }) => dispatch(ownProps.actionCreator(keyValueObject)),
})

const KeyValueEditor = connect(mapStateToProps, mapDispatchToProps)(KeyValueEditorComponent as any);
export default KeyValueEditor;
