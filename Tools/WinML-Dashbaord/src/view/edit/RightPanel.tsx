import * as Ajv from 'ajv';
import { JsonEditor } from 'jsoneditor-react';
import 'jsoneditor-react/es/editor.min.css';
import { Label } from 'office-ui-fabric-react/lib/Label';
import * as React from 'react';
import { connect } from 'react-redux';

import Collapsible from '../../components/Collapsible';
import { updateMetadataProps } from '../../datastore/actionCreators';
import IState, { IMetadataProps } from '../../datastore/state';
import MetadataSchema from '../../schema/Metadata';

import './Panel.css';

const ajv = new Ajv({ allErrors: true, verbose: true });

interface IComponentProperties {
    // Redux properties
    metadataProps: IMetadataProps,
    updateMetadataProps: typeof updateMetadataProps,
}

class RightPanelComponent extends React.Component<IComponentProperties, {}> {
    public render() {
        return (
            <div>
                <Label>Model</Label>
                <div className='Panel'>
                    <Collapsible label='Model metadata'>
                        <div key={JSON.stringify(this.props.metadataProps)}>
                            <JsonEditor
                                ajv={ajv}
                                schema={MetadataSchema}
                                value={this.props.metadataProps}
                                onChange={console.log}
                            />
                        </div>
                    </Collapsible>
                </div>
            </div>
        );
    }
}

const mapStateToProps = (state: IState) => ({
    metadataProps: state.metadataProps,
});

const mapDispatchToProps = {
    updateMetadataProps,
}

const RightPanel = connect(mapStateToProps, mapDispatchToProps)(RightPanelComponent);
export default RightPanel;
