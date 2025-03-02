
const pageLogin = {
    data: function () {
      return {
        name: null,
        pass: null,
        isPwd: true,
      };
    },
    methods: {
      ...Vuex.mapActions(["setAuthentication"]),
      onSubmit() {
        this.setAuthentication({ name: this.name, pass: this.pass });
      },
      onReset() {
        this.name = null;
        this.pass = null;
      },
    },
    template: /*html*/ `
    <q-page>
      <b-container title="Login">
        <div class="q-pa-lg">
    
        <q-form @submit="onSubmit" @reset="onReset" class="q-gutter-sm" >
          
                  <q-input filled v-model="name" label="Name" dense
                    lazy-rules :rules="[ val => val && val.length > 0 || 'Please type something']">
                  </q-input>
                      
                  <q-input v-model="pass" filled dense label="Password"
                    lazy-rules :rules="[ val => val && val.length > 0 || 'Please type something']"
                    :type="isPwd ? 'password' : 'text'">
                    
                    <template v-slot:append>
                      <q-icon
                        :name="isPwd ? 'icon-visibility_off' : 'icon-remove_red_eye'"
                        class="cursor-pointer"
                        @click="isPwd = !isPwd"
                      />
                    </template>
                  </q-input>
                      
                  <div>
                      <q-btn label="Enviar" type="submit" color="primary"/>
                      
                      <q-btn label="Reset" type="reset" color="primary" flat/>
                  </div>
                </q-form>
        
        </div >
      </b-container>
    </q-page>
    `,
  };